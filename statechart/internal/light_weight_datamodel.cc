// Copyright 2018 The StateChart Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "statechart/internal/light_weight_datamodel.h"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <functional>
#include <iterator>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include <glog/logging.h>

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "statechart/internal/function_dispatcher.h"
#include "statechart/internal/utility.h"
#include "statechart/logging.h"
#include "statechart/platform/map_util.h"
#include "statechart/platform/str_util.h"

using state_chart::internal::ArrayReferenceIterator;
using state_chart::internal::ArrayValueIterator;
using ::google::int64;

namespace state_chart {

namespace {

using namespace std::placeholders;

template <typename Arg1, typename Arg2, typename Arg3>
auto BindFront(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
    -> decltype(::std::bind(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2),
                            std::forward<Arg3>(arg3), _1, _2, _3, _4, _5)) {
  return ::std::bind(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2),
                     std::forward<Arg3>(arg3), _1, _2, _3, _4, _5);
}

class Token;
bool SubstituteUntilValue(const Json::Value& store, const Runtime* runtime,
                          FunctionDispatcher* dispatcher,
                          std::list<Token>* expr);

// Operators of expressions that delimit values/subexpressions.
const char* const kOperators[] = {
    ",", "(",  ")",  "[",  "]",  "+", "-",  "*",  "/",
    "<", "<=", "==", "!=", ">=", ">", "&&", "||", "!",
};

// Special values in the system.
const char* const kSpecialValues[] = {
    "true", "false", "null",
};

// A unary operation that stores (in arg2) the result of applying the operator
// (arg0) on a string token (arg1).
// Returns false if an error was encountered.
typedef std::function<bool(const Token&, const Token&, Token*)> UnaryOperation;

// A binary operation that stores (in arg3) the result of applying the operator
// (arg0) on two string tokens (arg1, arg2).
// Returns false if an error was encountered.
typedef std::function<bool(const Token&, const Token&, const Token&, Token*)>
    BinaryOperation;

// A predicate that returns true if the given Token matches.
typedef std::function<bool(const Token&)> TokenPredicate;

// A vector of token predicates.
typedef std::vector<TokenPredicate> TokenPredicates;

// Substitution methods used in SubstituteUntilValue() must have the same
// method signature as SubstituteMethod.
// Parameters in order:
// - The store for translating locations.
// - The pointer to associated runtime (if present).
// - The dispatcher for system function calls.
// - The tokenized expression
// - A pointer to an error flag.
// Return true if any substitutions occurred.
typedef std::function<bool(const Json::Value&, const Runtime*,
                           FunctionDispatcher*, std::list<Token>*, bool*)>
    SubstituteMethod;

// A special Json::Value that indicates a value does not exists.
const Json::Value& UndefinedJSON() {
  static const auto* undef_value =
      new Json::Value("__INTERNAL_UNDEFINED_VALUE__");
  return *undef_value;
}

// Return true if a value is found in a C-style string array.
template <class CStringArray>
bool InCStringArray(const CStringArray& cstr_array, const string& str) {
  for (const auto& cstr : cstr_array) {
    if (str == cstr) {
      return true;
    }
  }
  return false;
}

// Return if a string is an operator.
bool IsOperatorString(const string& str) {
  return InCStringArray(kOperators, str);
}

// Search for a value in the store. If found, set 'value_reference' to the
// pointer to that value and return true.
bool FindValueInStore(const Json::Value& store, const string& location,
                      const Json::Value** value_reference) {
  RETURN_FALSE_IF(value_reference == nullptr);
  Json::Path path(location);
  Json::Value result = path.resolve(store, UndefinedJSON());
  if (result == UndefinedJSON()) {
    return false;
  }
  *value_reference = &path.resolve(store);
  return true;
}

// Convert a JSON value to a compact formatted string. Quotes values that
// represent string literals if 'quote_string' is true.
string ValueToString(const Json::Value& value, bool quote_string) {
  if (value.isObject() || value.isArray()) {
    string result = Json::FastWriter().write(value);
    // Last char is a newline.
    result.pop_back();
    return result;
  } else if (value.isNull()) {
    return "null";
  } else if (quote_string && value.isString()) {
    return Quote(value.asString());
  } else {
    return value.asString();
  }
}

// Convert a JSON value to a compact formatted string. Does not quote values
// that represent string literals.
string ValueToString(const Json::Value& value) {
  return ValueToString(value, false);
}

//------------------------------------------------------------------------------
// An omni data type class representing an expression token in the expression
// language that can be either a value (of any allowed valid type), a reference
// to a value, an operator, or a system function.
class Token {
 public:
  // Create the token from an expression ('expr'), a 'store', and a function
  // dispatcher.
  // Only some types of expressions are valid. First, the value literal
  // expression will create a value token, i.e., a constant.
  // Second, an expression that references the store will create a reference
  // Token. A reference token transparently dereferences the store location
  // that the expression refers to when Value() is called.
  // Third, a known operator creates an 'Operator' token.
  // Fourth, a function name that exists in the 'dispatcher' creates a
  // 'SystemFunction' token.
  // An optional 'is_error' flag is used to indicate if there an
  // unknown expression that is not one of the above mentioned types.
  static Token Create(const Json::Value& store,
                      const FunctionDispatcher& dispatcher, string expr,
                      bool* is_error);

  // Default constructor. Creates an empty token (not a JSON null value).
  // Used to create empty tokens for pass by reference.
  Token() : reference_(nullptr) {}

  // Constructor for operators.
  explicit Token(const string& op) : reference_(nullptr), operator_(op) {}

  // Constructor for values.
  explicit Token(const Json::Value& value)
      : value_(new Json::Value(value)), reference_(nullptr) {}

  // Constructor for references.
  explicit Token(const Json::Value* reference) : reference_(reference) {}

  // Copy constructor.
  Token(const Token& other);

  // Assignment using copy-swap idiom.
  Token& operator=(Token other) {
    Swap(&other);
    return *this;
  }

  ~Token() = default;

  void Swap(Token* other);

  // Returns the value of this Token. Does transparent dereferencing if this is
  // a reference.
  // Returns UndefinedJSON() if !IsValue().
  const Json::Value& Value() const {
    if (!IsValue()) {
      LOG(DFATAL) << "Returning default Json::Value for token: "
                  << DebugString();
      return UndefinedJSON();
    }
    return IsReference() ? *reference_ : *value_;
  }

  // Returns the mutable Value of this Token.
  // Returns nullptr if !IsValue() || IsReference().
  Json::Value* MutableValue() {
    RETURN_NULL_IF_MSG(
        !IsValue() || IsReference(),
        absl::StrCat("Returning nullptr for token: ", DebugString()));
    return value_.get();
  }

  // Returns the operator, which is empty if !IsOperator().
  const string& Operator() const { return operator_; }

  // Returns name of the system function which is empty if !IsSystemFunction().
  const string& SystemFunction() const { return system_function_; }

  // Returns true if this is a value literal or a reference, i.e., Value() may
  // be called.
  bool IsValue() const { return value_ != nullptr || IsReference(); }
  bool IsReference() const { return reference_ != nullptr; }
  bool IsOperator() const { return !operator_.empty(); }
  bool IsSystemFunction() const { return !system_function_.empty(); }

  // Returns true if a value is internally represented as an integer.
  // Returns false if !IsValue().
  bool IsInteger() const {
    RETURN_FALSE_IF_MSG(
        !IsValue(),
        absl::StrCat("Returning False; for non-value token: ", DebugString()));
    return Value().isNumeric() && Value().type() != Json::realValue;
  }

  // Prints out the contents of this Token.
  string DebugString() const;

  // Convert the value to a boolean type. Handles the cases that
  // Json::Value::asBool() do not handle.
  // Returns false if !IsValue().
  bool ToBool() const;

 private:
  // Holds the value if this token is a literal value, otherwise nullptr.
  std::unique_ptr<Json::Value> value_;
  // Holds the reference if this token is a reference (from a location),
  // otherwise nullptr. Never owned.
  const Json::Value* reference_;
  // Holds the operator expression if this token is an operator, otherwise
  // empty.
  string operator_;
  // Holds the system function name if this token is a system function,
  // otherwise empty.
  string system_function_;
};

// static
Token Token::Create(const Json::Value& store,
                    const FunctionDispatcher& dispatcher, string expr,
                    bool* is_error) {
  absl::StripAsciiWhitespace(&expr);
  if (is_error != nullptr) {
    *is_error = false;
  }
  Json::Reader reader;
  Json::Value value_root;
  const Json::Value* value_reference = nullptr;
  int64 value_i = 0;
  double value_d = 0;
  if (expr.empty() || expr == "null") {
    // Null value.
    DVLOG(1) << "Created null: " << expr;
    return Token(Json::Value());
  } else if (expr == "true") {
    // True value.
    DVLOG(1) << "Created true: " << expr;
    return Token(Json::Value(true));
  } else if (expr == "false") {
    // False value.
    DVLOG(1) << "Created false: " << expr;
    return Token(Json::Value(false));
  } else if (IsOperatorString(expr)) {
    // Operator.
    DVLOG(1) << "Created operator: " << expr;
    return Token(expr);
  } else if (absl::SimpleAtoi(expr, &value_i)) {
    // Integer.
    DVLOG(1) << "Created integer: " << expr;
    return Token(Json::Value(value_i));
  } else if (absl::SimpleAtod(expr, &value_d)) {
    // Double.
    DVLOG(1) << "Created double: " << expr;
    return Token(Json::Value(value_d));
  } else if (IsQuotedString(expr)) {
    // String.
    DVLOG(1) << "Created string: " << expr;
    return Token(Json::Value(Unquote(expr)));
  } else if ((MaybeJSONArray(expr) || MaybeJSON(expr)) &&
             reader.parse(expr, value_root, false)) {
    // MaybeJSON*() is required as the reader will parse "1 + 1" as valid
    // Json::Value string.
    // Object or array.
    DVLOG(1) << "Created object/array: " << expr;
    return Token(value_root);
  } else if (expr == "In") {
    Token token;
    token.system_function_ = expr;
    DVLOG(1) << "Created system function: " << expr;
    return token;
  } else if (dispatcher.HasFunction(expr)) {
    // Note that a system function name takes precedence over a location name.
    // Hence attempting to declare or assign to a system function name will
    // always result in an error.
    Token token;
    token.system_function_ = expr;
    DVLOG(1) << "Created system function: " << expr;
    return token;
  } else if (FindValueInStore(store, expr, &value_reference)) {
    // Reference
    DVLOG(1) << "Created reference: " << expr;
    return Token(value_reference);
  } else {
    if (is_error != nullptr) {
      *is_error = true;
    }
    return Token();
  }
}

Token::Token(const Token& other)
    : reference_(other.reference_),
      operator_(other.operator_),
      system_function_(other.system_function_) {
  if (other.value_ != nullptr) {
    value_.reset(new Json::Value(*other.value_));
  }
}

void Token::Swap(Token* other) {
  if (this != other) {
    value_.swap(other->value_);
    std::swap(reference_, other->reference_);
    operator_.swap(other->operator_);
    system_function_.swap(other->system_function_);
  }
}

string Token::DebugString() const {
  if (IsOperator()) {
    return absl::StrCat("OP:", Operator());
  }
  if (IsReference()) {
    return absl::StrCat("REF:", ValueToString(Value()));
  }
  if (IsValue()) {
    return ValueToString(Value(), true);
  }
  if (IsSystemFunction()) {
    return absl::StrCat("SYS:", SystemFunction());
  }
  return "<EMPTY>";
}

bool Token::ToBool() const {
  RETURN_FALSE_IF(!IsValue());
  if (Value().isObject() || Value().isArray()) {
    return true;
  } else if (Value().isString()) {
    return strlen(Value().asCString()) > 0;
  }
  return Value().asBool();
}
//------------------------------------------------------------------------------

// Create a debug string for a unary operation.
string UnaryDebugString(const Token& op, const Token& a) {
  return absl::StrCat(op.DebugString(), " ", a.DebugString());
}

// Create a debug string for a binary operation.
string BinaryDebugString(const Token& op, const Token& a, const Token& b) {
  return absl::StrCat(a.DebugString(), " ", op.DebugString(), " ",
                      b.DebugString());
}

// Convert a container of Tokens into a vector of their DebugStrings.
template <class TokenContainer>
std::vector<string> ListDebugStrings(const TokenContainer& c) {
  std::vector<string> result;
  for (const auto& token : c) {
    result.push_back(token.DebugString());
  }
  return result;
}

// Returns true if an operator is a relational comparison operator.
bool IsRelationalOp(const Token& op) {
  static const char* const kRelationalOps[] = {"<", ">", "<=", ">="};
  return InCStringArray(kRelationalOps, op.Operator());
}

// Returns true if an operator is an equality comparison operator.
bool IsEqualityOp(const Token& op) {
  static const char* const kEqualityOps[] = {"==", "!="};
  return InCStringArray(kEqualityOps, op.Operator());
}

// A meta function that identifies an infix binary operator ('op') and operands
// and substitutes them if the operands are values. Otherwise an error will be
// reported. Returns true if a substitution occurred. The binary operator is
// assumed to be left associative.
// If the first two parameters are bound, the resulting function is a
// SubstituteMethod.
bool SubInfixBinaryOpMeta(const TokenPredicate& op_match,
                          BinaryOperation op_function, const Json::Value&,
                          const Runtime* /* unused */,
                          FunctionDispatcher* /* unused */,
                          std::list<Token>* expr, bool* is_error) {
  *is_error = false;
  if (expr->size() < 3) {
    return false;
  }

  TokenPredicates predicates{
      &Token::IsValue,
      [&op_match](const Token& x) { return x.IsOperator() && op_match(x); },
      &Token::IsValue};

  bool substituted = false;
  auto it = expr->begin();
  auto it_end = expr->end();
  // Note: Assignment in condition.
  while (it_end != (it = SearchWithPredicates(it, it_end, predicates))) {
    // Loop invariants.
    LOG_IF(DFATAL, std::distance(it, it_end) < 0)
        << "Parsing of binary op failed.";
    LOG_IF(DFATAL, static_cast<::std::size_t>(std::distance(it, it_end)) <
                       predicates.size())
        << "Parsing of binary op failed.";

    auto it_first = it++;
    auto it_op = it++;
    Token result;
    DVLOG(1) << "BinOp: " << BinaryDebugString(*it_op, *it_first, *it);
    if (!op_function(*it_op, *it_first, *it, &result)) {
      *is_error = true;
      break;
    }
    // Store result at it_first.
    it_first->Swap(&result);
    // Erase 'it_op' and 'it'.
    expr->erase(it_op, ++it);
    // Start from the new result at 'it_first'.
    it = it_first;
    substituted = true;
  }
  return substituted;
}

// A meta-function that runs a binary operator on two numeric operands such
// that if both operands are integers, the integer operator will be used.
// Alternatively, if one (or both) operand is a double, the other will be
// promoted to a double and the double operation will be used.
template <class Int64_Op, class DoubleOp>
bool NumericOperationMeta(Int64_Op int_op, DoubleOp dbl_op, const Token& a,
                          const Token& b, Token* result) {
  const auto& va = a.Value();
  const auto& vb = b.Value();
  // Check for the numeric type, handle integer only operations. Convert boolean
  // values to integers or doubles if possible.
  if ((a.IsInteger() || va.isBool()) && (b.IsInteger() || vb.isBool())) {
    *result = Token(Json::Value(int_op(va.asInt(), vb.asInt())));
  } else if ((va.isDouble() || va.isBool()) && (vb.isDouble() || vb.isBool())) {
    *result = Token(Json::Value(dbl_op(va.asDouble(), vb.asDouble())));
  } else {
    LOG(INFO) << "Unsupported operand type: " << a.DebugString() << ", "
              << b.DebugString();
    return false;
  }
  return true;
}

// Evaluate the '+' operator.
// The semantic rules in order of precedence are:
// - If any operand is a string, '+' promotes both operands to strings and
//   does concatenation.
// - If both operands are integers, the result is an integer sum.
// - If any operand is a double, the result is a sum of doubles.
bool PlusOperation(const Token& a, const Token& b, Token* result) {
  if (a.Value().isString() || b.Value().isString()) {
    *result =
        Token(Json::Value(ValueToString(a.Value()) + ValueToString(b.Value())));
  } else {
    return NumericOperationMeta(std::plus<int>(), std::plus<double>(), a, b,
                                result);
  }
  return true;
}

// Evaluate the '-' operator. Only works for numbers.
bool MinusOperation(const Token& a, const Token& b, Token* result) {
  return NumericOperationMeta(std::minus<int>(), std::minus<double>(), a, b,
                              result);
}

// A BinaryOperation. Selects the plus or minus operation based on 'op'.
bool AdditiveOperation(const Token& op, const Token& a, const Token& b,
                       Token* result) {
  if (op.Operator() == "+") {
    return PlusOperation(a, b, result);
  } else if (op.Operator() == "-") {
    return MinusOperation(a, b, result);
  } else {
    LOG(DFATAL) << "Unrecognized operator: " << op.DebugString();
    return false;
  }
}

// Evaluate the '*' operator. Only works for numbers.
bool MultiplyOperation(const Token& a, const Token& b, Token* result) {
  return NumericOperationMeta(std::multiplies<int>(), std::multiplies<double>(),
                              a, b, result);
}

// Evaluate the '/' operator. Only works for numbers.
bool DivideOperation(const Token& a, const Token& b, Token* result) {
  // Unconvertable to double case will be handled after this.
  if (b.Value().isNumeric() && b.Value().asDouble() == 0) {
    LOG(INFO) << "divide by zero: " << a.DebugString() << " / "
              << b.DebugString();
    return false;
  }
  return NumericOperationMeta(std::divides<int>(), std::divides<double>(), a, b,
                              result);
}

// A BinaryOperation. Selects the multiplies or divide operation based on 'op'.
bool MultiplicativeOperation(const Token& op, const Token& a, const Token& b,
                             Token* result) {
  if (op.Operator() == "*") {
    return MultiplyOperation(a, b, result);
  } else if (op.Operator() == "/") {
    return DivideOperation(a, b, result);
  } else {
    LOG(DFATAL) << "Unrecognized operator: " << op.DebugString();
    return false;
  }
}

// A BinaryOperation. Compares strings based on 'cmp_op'.
// Should be called after NumericComparison as it checks for invalid string with
// numeric comparisons.
// Unlike Javascript, string operands are not promoted to numbers when compared
// with a numeric operand. Hence string with numeric comparison is unsupported.
bool StringComparison(const Token& cmp_op, const Token& a, const Token& b,
                      Token* result) {
  const auto& va = a.Value();
  const auto& vb = b.Value();
  if (!va.isString() || !vb.isString()) {
    return false;
  }
  bool result_bool = false;
  if (cmp_op.Operator() == "==") {
    result_bool = (va == vb);
  } else if (cmp_op.Operator() == "!=") {
    result_bool = (va != vb);
  } else if (cmp_op.Operator() == "<") {
    result_bool = (va < vb);
  } else if (cmp_op.Operator() == "<=") {
    result_bool = (va <= vb);
  } else if (cmp_op.Operator() == ">") {
    result_bool = (va > vb);
  } else if (cmp_op.Operator() == ">=") {
    result_bool = (va >= vb);
  } else {
    LOG(DFATAL) << "Unrecognized comparison: "
                << BinaryDebugString(cmp_op, a, b);
    return false;
  }
  *result = Token(Json::Value(result_bool));
  DVLOG(1) << "StringComparison result = " << result->DebugString();
  return true;
}

// A BinaryOperation.
// This function takes in two values and compares them using semantics for the
// given 'cmp_op' for 'int64' and 'double'.
bool NumericComparison(const Token& cmp_op, const Token& a, const Token& b,
                       Token* result) {
  static const auto* const kComparatorTable =
      new std::map<string, std::pair<std::function<bool(int, int)>,
                                std::function<bool(double, double)>>>{
          {"==", {std::equal_to<int>(), std::equal_to<double>()}},
          {"!=", {std::not_equal_to<int>(), std::not_equal_to<double>()}},
          {"<", {std::less<int>(), std::less<double>()}},
          {"<=", {std::less_equal<int>(), std::less_equal<double>()}},
          {">", {std::greater<int>(), std::greater<double>()}},
          {">=", {std::greater_equal<int>(), std::greater_equal<double>()}},
      };

  const auto& va = a.Value();
  const auto& vb = b.Value();

  // Do not do anything if arguments are not numeric.
  if (!va.isNumeric() || !vb.isNumeric()) {
    return false;
  }
  const auto& iterator = kComparatorTable->find(cmp_op.Operator());
  // Fail if invalid comparison, this indicates a setup error.
  RETURN_FALSE_IF(iterator == kComparatorTable->end());
  const auto& comparators = iterator->second;
  return NumericOperationMeta(comparators.first, comparators.second, a, b,
                              result);
}

// A BinaryOperation.
// Does comparison operators and type promotion.
bool ComparisonOperation(const Token& cmp_op, const Token& a, const Token& b,
                         Token* result) {
  if (!a.IsValue() || !b.IsValue()) {
    LOG(INFO) << "Operand error: " << BinaryDebugString(cmp_op, a, b);
    return false;
  }

  const auto& va = a.Value();
  const auto& vb = b.Value();
  const auto& op = cmp_op.Operator();

  if (va.isBool() && vb.isBool()) {
    // Only equality comparators supported for bool. No promotion allowed.
    if (op == "==") {
      *result = Token(Json::Value(a.ToBool() == b.ToBool()));
      return true;
    } else if (op == "!=") {
      *result = Token(Json::Value(a.ToBool() != b.ToBool()));
      return true;
    }
  } else if (va.isNull() || vb.isNull()) {
    // Only equality comparators supported for null.
    if (IsEqualityOp(cmp_op)) {
      // True when both operands are null and comparator is '=='.
      // Or when comparator is '!=' and only one operand is null.
      if ((op == "==" && va.isNull() && vb.isNull()) ||
          (op == "!=" && (!va.isNull() || !vb.isNull()))) {
        *result = Token(Json::Value(true));
        return true;
      }
      *result = Token(Json::Value(false));
      return true;
    }
  } else if (NumericComparison(cmp_op, a, b, result)) {
    return true;
  } else if (StringComparison(cmp_op, a, b, result)) {
    // String promotion.
    return true;
  }

  LOG(INFO) << "Invalid comparison: " << BinaryDebugString(cmp_op, a, b);
  return false;
}

// A BinaryOperation. Logical boolean AND.
bool LogicalAndOperation(const Token& op, const Token& a, const Token& b,
                         Token* result) {
  RETURN_FALSE_IF("&&" != op.Operator());
  *result = Token(Json::Value(a.ToBool() && b.ToBool()));
  return true;
}

// A BinaryOperation. Logical boolean OR.
bool LogicalOrOperation(const Token& op, const Token& a, const Token& b,
                        Token* result) {
  RETURN_FALSE_IF("||" != op.Operator());
  *result = Token(Json::Value(a.ToBool() || b.ToBool()));
  return true;
}

// A meta function that takes in an operator matcher, 'op_matcher', and
// performs the corresponding UnaryOperation, 'op_function', substituting the
// sub-expression with the result. The unary operator is assumed to be right
// associative.
// If the first two parameters are bound, the resulting function is a
// SubstituteMethod.
bool SubUnaryOpMeta(const TokenPredicate& op_match, UnaryOperation op_function,
                    const Json::Value&, const Runtime* /* unused */,
                    FunctionDispatcher* /* unused */, std::list<Token>* expr,
                    bool* is_error) {
  *is_error = false;
  if (expr->size() < 2) {
    return false;
  }

  bool substituted = false;

  // To compute with right associativity, process the expression in reverse.
  auto it = expr->rbegin();
  auto it_end = expr->rend();
  // Note that the predicates are in reverse order -- value, operator.
  TokenPredicates predicates{
      &Token::IsValue,
      [&op_match](const Token& x) { return x.IsOperator() && op_match(x); },
  };

  while (it_end != (it = SearchWithPredicates(it, it_end, predicates))) {
    // Loop invariants.
    LOG_IF(DFATAL, static_cast<::std::size_t>(std::distance(it, it_end)) <
                       predicates.size())
        << "Parsing of unary op failed.";

    // Move 'it' to the operator.
    auto it_value = it++;
    // Move 'it' to one after the operator.
    auto it_op = it++;
    // Skip if 'it' is a value to disambiguate binary '-' operator.
    if (it != it_end && it->IsValue()) {
      continue;
    }
    DVLOG(1) << "Found unary operation: " << it_op->DebugString()
             << it_value->DebugString();
    Token result;
    if (!op_function(*it_op, *it_value, &result)) {
      *is_error = true;
      return false;
    }
    // Store the result in the operator position.
    it_op->Swap(&result);
    // Erase the value position, 'it_op.base()' is the un-reversed iterator to
    // the value. Note that 'it_op' is invalidated after this.
    expr->erase(it_op.base());
    // Continue from the result (new value) position.
    --it;
    substituted = true;
  }
  return substituted;
}

// A UnaryOperation. Evaluates the unary '-' operator.
// Only works on numeric operands.
bool UnaryMinusOperation(const Token& op, const Token& value, Token* result) {
  RETURN_FALSE_IF(op.Operator() != "-");
  const auto& v = value.Value();
  if (!v.isNumeric()) {
    LOG(INFO) << "Operand is not a number: " << UnaryDebugString(op, value);
    return false;
  }
  *result = Token(v.isDouble() ? Json::Value(-v.asDouble())
                               : Json::Value(-v.asInt()));
  return true;
}

// A UnaryOperation. Evaluates boolean NOT.
bool LogicalNotOperation(const Token& op, const Token& value, Token* result) {
  RETURN_FALSE_IF(op.Operator() != "!");
  *result = Token(Json::Value(!value.ToBool()));
  return true;
}

// Check if an expression sequence is a valid sequence of values separated by
// ','. Returns true for a valid sequence or empty expression.
// 'InputIterator' must have value type 'Token'.
template <class InputIterator>
bool IsValueSequence(InputIterator first, InputIterator last) {
  if (first == last) {
    return true;
  }
  auto size = std::distance(first, last);
  // First element must be a value, sequence must have odd number of tokens.
  if (!first->IsValue() || size % 2 == 0) {
    return false;
  }
  // We check for a following sequence of ',', 'value', ',', 'value', ...
  // 'it_op' points to the ',' operator.
  // 'first' points to the value.
  auto it_op = first;
  ++it_op;
  while (it_op != last) {
    std::advance(first, 2);
    if (it_op->Operator() != "," || !first->IsValue()) {
      return false;
    }
    // Move 'it_op' to next ','.
    std::advance(it_op, 2);
  }
  return true;
}

template <class T>
bool PairEquals(const std::pair<T, T>& x) {
  return x.first == x.second;
}

// Recursively evaluates the sub expressions in the outermost parentheses.
// If the first three parameters are bound, this is a SubstituteMethod.
// Parentheses may also occur for system function calls. In this case, and in
// the case of array or dictionary element accesses, the arguments in the
// parentheses are evaluated and replaced with a value sequence but the
// parentheses are not stripped.
bool SubstituteParentheses(const Json::Value& store, const Runtime* runtime,
                           FunctionDispatcher* dispatcher,
                           std::list<Token>* expr, bool* is_error) {
  *is_error = false;

  // A string that holds the current type, either of '(' or '['. The type
  // will be auto-detected and set by 'is_start' below. Then, 'is_end' will
  // adjust its detection type based on 'parenthesis_type'.
  string parenthesis_type;

  // This will match the start parentheses as either type and set the type.
  auto is_start = [&parenthesis_type](const Token& x) {
    if (!x.IsOperator()) {
      return false;
    }
    // Detect type if empty.
    if (parenthesis_type.empty()) {
      if (x.Operator() == "(" || x.Operator() == "[") {
        parenthesis_type = x.Operator();
        return true;
      }
      return false;
    }
    // Use detected type if non-empty.
    if (parenthesis_type == x.Operator()) {
      return true;
    }
    return false;
  };
  // End will return true for either parenthesis type until 'is_start' has
  // detected the parenthesis type. After which 'is_end' will only match based
  // on the detected type. This allows for early error detection if a closing
  // parenthesis is detected before any opening parenthesis.
  auto is_end = [&parenthesis_type](const Token& x) {
    if (!x.IsOperator()) {
      return false;
    }
    if (parenthesis_type.empty()) {
      // A match here indicates an error detectable within
      // FindOuterMostParenthesis().
      if (x.Operator() == ")" || x.Operator() == "]") {
        return true;
      }
      return false;
    }
    if (parenthesis_type == "(" && x.Operator() == ")") {
      return true;
    }
    if (parenthesis_type == "[" && x.Operator() == "]") {
      return true;
    }
    return false;
  };

  bool substituted = false;
  auto it_begin = expr->begin();
  auto it = it_begin;
  auto it_end = expr->end();
  auto range = std::make_pair(it_end, it_end);
  std::list<Token> sub_expr;
  // Note assignment in condition.
  while (!PairEquals(
      range = FindOuterMostParentheses(it, it_end, is_start, is_end))) {
    // Check loop invariant.
    LOG_IF(DFATAL, !(parenthesis_type == "(" || parenthesis_type == "["))
        << parenthesis_type;
    auto first = range.first;
    ++first;
    auto before_first = range.first;
    if (range.first != it_begin) {
      --before_first;
    }
    // Nothing to do if it is function call or element access and the expression
    // within the parenthesis is a value sequence.
    if (IsValueSequence(first, range.second)) {
      if (parenthesis_type == "[" ||
          (parenthesis_type == "(" && before_first->IsSystemFunction())) {
        it = ++range.second;
        parenthesis_type.clear();
        continue;
      }
    }
    // Moves [first, range.second) to 'sub_expr'.
    sub_expr.clear();
    sub_expr.splice(sub_expr.end(), *expr, first, range.second);
    // There may not be an empty expression within '[]' parenthesis.
    if (sub_expr.empty() && parenthesis_type == "[") {
      DVLOG(1) << "Error: empty expression between '[]'.";
      *is_error = true;
      return true;
    }
    // There may be an empty expression within '()' parenthesis, e.g., a
    // function call with no arguments.
    if (!sub_expr.empty() &&
        !SubstituteUntilValue(store, runtime, dispatcher, &sub_expr)) {
      *is_error = true;
      return true;
    }
    // Move the entire result back into the expression.
    expr->splice(range.second, sub_expr);
    substituted = true;
    // Remove parenthesis or not depending on type.
    // Continue detection after the parenthesis group.
    if (parenthesis_type == "(" && !before_first->IsSystemFunction()) {
      expr->erase(range.first);
      it = expr->erase(range.second);
    } else {  // parenthesis_type == "[" or a function call.
      it = ++range.second;
    }
    parenthesis_type.clear();
  }
  return substituted;
}

// A SubstituteMethod.
// Substitute element accesses on references with new reference Tokens.
bool SubstituteElementAccess(const Json::Value& store,
                             const Runtime* /* unused */,
                             FunctionDispatcher* /* unused */,
                             std::list<Token>* expr, bool* is_error) {
  // Matcher for element access.
  static const auto* const kElementAccessMatcher = new TokenPredicates{
      [](const Token& x) {
        return x.IsValue() && (x.Value().isObject() || x.Value().isArray());
      },
      [](const Token& x) { return x.IsOperator() && x.Operator() == "["; },
      &Token::IsValue,
      [](const Token& x) { return x.IsOperator() && x.Operator() == "]"; },
  };

  *is_error = false;
  bool substituted = false;
  string location;
  auto it = expr->begin();
  auto it_end = expr->end();
  // Note assignment in condition.
  while (it_end !=
         (it = SearchWithPredicates(it, it_end, *kElementAccessMatcher))) {
    // Stores the reference of the object/array. Will be replaced with the value
    // of the element access.
    auto it_ref = it;
    // Move 'it' to element index value.
    std::advance(it, 2);
    if (it_ref->Value().isArray()) {
      // Special built-in array property, 'length'.
      // Unlike Javascript, 'length' is not assignable since it is not a
      // store location reference, but a literal integer value.
      if (ValueToString(it->Value()) == "length") {
        *it_ref = Token(Json::Value(it_ref->Value().size()));
      } else if (!it->Value().isIntegral() || it->Value().asInt() < 0 ||
                 static_cast<::std::size_t>(it->Value().asInt()) >=
                     it_ref->Value().size()) {
        // Error if using non-integral value to access an array.
        *is_error = true;
        DVLOG(1) << "Accessing array at: " << it_ref->DebugString()
                 << ", with invalid index: " << it->DebugString();
        return substituted;
      } else if (it_ref->IsReference()) {
        // Array reference index element access.
        *it_ref = Token(&it_ref->Value()[it->Value().asInt()]);
      } else {
        // Array value index element access, copy element.
        *it_ref = Token(it_ref->Value()[it->Value().asInt()]);
      }
    } else if (it_ref->Value().isObject()) {
      location = ValueToString(it->Value());
      if (!it_ref->Value().isMember(location)) {
        *is_error = true;
        DVLOG(1) << "Accessing object at: " << it_ref->DebugString()
                 << ", with invalid field: " << it->DebugString();
        return substituted;
      }
      if (it_ref->IsReference()) {
        // Object reference element access.
        *it_ref = Token(&it_ref->Value()[location]);
      } else {
        // Object value element access, copy element.
        *it_ref = Token(it_ref->Value()[location]);
      }
    } else {
      *is_error = true;
      DVLOG(1) << "Element access error, reference is not an array or object: "
               << it_ref->DebugString();
      return substituted;
    }
    substituted = true;
    // Result of element access has been stored in 'it_ref'.
    // Erase elements from '[' to ']', backup old position in 'it_new'.
    auto it_new = it_ref++;  // Move to '['.
    std::advance(it, 2);     // Move to one after ']'.
    expr->erase(it_ref, it);
    // Restart from 'it_new' so that 'foo[1][2]' can be processed.
    it = it_new;
  }  // while
  return substituted;
}

// A substitute method. Processes a system function call and returns the result.
// Expects that all arguments to functions have been evaluated to values,
// otherwise returns false.
bool SubstituteSystemFunctionCalls(const Json::Value& store,
                                   const Runtime* runtime,
                                   FunctionDispatcher* dispatcher,
                                   std::list<Token>* expr, bool* is_error) {
  // Matcher for start of a function call.
  static const auto* const kStartFunctionCallMatcher = new TokenPredicates{
      std::mem_fn(&Token::IsSystemFunction),
      [](const Token& x) { return x.IsOperator() && x.Operator() == "("; },
  };
  *is_error = false;
  bool substituted = false;
  auto it = expr->begin();
  auto it_end = expr->end();
  auto range = std::make_pair(it_end, it_end);
  std::list<Token> arg_sequence;
  std::vector<const Json::Value*> arguments;
  // Search for the start of a function call.
  // Note the assignment in the condition.
  while (it_end !=
         (it = SearchWithPredicates(it, it_end, *kStartFunctionCallMatcher))) {
    // Search for the end of the function call.
    range = FindOuterMostParentheses(
        it, it_end, kStartFunctionCallMatcher->at(1),
        [](const Token& x) { return x.Operator() == ")"; });
    if (PairEquals(range)) {
      DVLOG(1) << "Invalid function call syntax for: " << it->DebugString();
      *is_error = true;
      return substituted;
    }
    // Advance to the first argument.
    ++range.first;
    arg_sequence.clear();
    arg_sequence.splice(arg_sequence.end(), *expr, range.first, range.second);
    arguments.clear();
    if (!arg_sequence.empty()) {
      substituted = true;
      RETURN_FALSE_IF_MSG(
          !IsValueSequence(arg_sequence.begin(), arg_sequence.end()),
          absl::StrCat(
              "Invalid argument list for function call: ", it->DebugString(),
              absl::StrJoin(ListDebugStrings(arg_sequence), " ")));

      for (const auto& token : arg_sequence) {
        if (token.IsValue()) {
          arguments.push_back(&token.Value());
        }
      }
    }
    Token return_value((Json::Value()));
    // return_value.MutableValue() will be non-NULL.
    if (it->SystemFunction() == "In") {
      if (runtime == nullptr || arguments.size() != 1 ||
          !arguments[0]->isString()) {
        DVLOG(1) << "Invalid call to function In("
                 << absl::StrJoin(ListDebugStrings(arg_sequence), ",") << ")\n"
                 << "Needs a valid Runtime and a single string argument.";
        *is_error = true;
        return substituted;
      }
      const string& state_id = arguments[0]->asString();
      *return_value.MutableValue() =
          Json::Value(runtime->IsActiveState(state_id));
    } else if (!dispatcher->Execute(it->SystemFunction(), arguments,
                                    return_value.MutableValue())) {
      *is_error = true;
      DVLOG(1) << "Error executing system function call: "
               << it->SystemFunction() << "("
               << absl::StrJoin(ListDebugStrings(arg_sequence), ",") << ")";
      return substituted;
    }
    // Substitute the function call with the result.
    // Erase the function name and '(' tokens. The argument sequence was
    // previously spliced out.
    it = expr->erase(it, range.second);
    // Replace the ')' token with the result.
    it->Swap(&return_value);
    // Start from one after the result.
    ++it;
  }
  return substituted;
}

// Hackish way to process 'Math.random()'.
// Seed a random generator with the current millisecond and draw real
// numbers uniformly within [0, 1).
void ComputeRandom(std::list<string>* expr) {
  static const auto* const kMathRandomExpr =
      new std::vector<string>{"Math.random", "(", ")"};
  std::unique_ptr<std::default_random_engine> lazy_generator;

  auto it = expr->begin();
  const auto it_end = expr->end();
  while (it_end !=
         // Assignment.
         (it = std::search(it, it_end, kMathRandomExpr->begin(),
                           kMathRandomExpr->end()))) {
    if (!lazy_generator) {
      lazy_generator.reset(
          new std::default_random_engine(time(nullptr) % 1000));
    }

    auto random_expr_start = it;
    std::advance(it, kMathRandomExpr->size());
    // Store result in the 'Math.random' node.
    *random_expr_start = absl::StrCat(
        std::uniform_real_distribution<>()(*lazy_generator));
    // Erase the excess nodes.
    ++random_expr_start;
    it = expr->erase(random_expr_start, it);
  }
}

// This preprocesses the expression before calling SubstituteUntilValue.
// It is used to convert the token values to standard form. It does the
// following conversions:
// - All single quoted strings to double quoted strings.
// - Substitute "Math.random()" with a random double in [0, 1).
// - All subpath expressions, ".<some_path>", occurring after an element access
//   are converted to element access tokens: "[", "<some_path>", "]". For
//   example, 'goo.foo[0].bar' becomes 'goo.foo[0]["bar"]'.
// - Replaces built-in object properties with element access. For example,
//   'some_array.length' becomes 'some_array["length"]'.
void PresubstituteStringTokens(const Json::Value& store,
                               std::list<string>* expr) {
  string object_location;
  for (auto it = expr->begin(), it_end = expr->end(); it != it_end; ++it) {
    const Json::Value* value_reference = nullptr;
    // Replace all singly quoted strings with double quotes.
    if (IsQuotedString(*it, '\'')) {
      *it = Quote(Unquote(*it, '\''));
    } else if (::absl::StartsWith(*it, ".")) {
      std::vector<absl::string_view> path_tokens =
          absl::StrSplit(*it, ".", absl::SkipEmpty());
      for (const auto& path_token : path_tokens) {
        expr->insert(it, "[");
        expr->insert(it, Quote(string(path_token)));
        expr->insert(it, "]");
      }
      it = expr->erase(it);
      --it;  // Move back one step since 'it' is incremented by loop.
    } else if (absl::EndsWith(*it, ".length")) {
      // Built-in length property replacement for arrays.
      object_location = it->substr(0, it->size() - strlen(".length"));
      if (FindValueInStore(store, object_location, &value_reference) &&
          value_reference->isArray()) {
        it->swap(object_location);
        // Insert after the object location.
        ++it;
        expr->insert(it, "[");
        expr->insert(it, Quote("length"));
        expr->insert(it, "]");
        --it;  // Move back to "]" since 'it' is incremented by loop.
      }
    }
  }
  // Process 'Math.random()' as if it is a macro substitution.
  ComputeRandom(expr);
}

// Returns the SubstituteMethods for the operators in substitution order.
const std::vector<SubstituteMethod>& GetSubstituteMethods() {
  // This list of substitutions defines the substitution order.
  static const auto* const kSubstituteMethods = new std::vector<
      SubstituteMethod>{
      // Recursively evaluate all parentheses.
      &SubstituteParentheses,
      // Do all system calls.
      &SubstituteSystemFunctionCalls,
      // Resolve all element access.
      &SubstituteElementAccess,
      // Unary minus takes highest precedence among arithmetic operations.
      // TODO(qplau): Combine unary negation and logical not if this precedence
      // poses a problem. Both are of equal precedence in JavaScript.
      BindFront(&SubUnaryOpMeta,
                [](const Token& op) { return op.Operator() == "-"; },
                &UnaryMinusOperation),
      // Do logical NOT.
      BindFront(&SubUnaryOpMeta,
                [](const Token& op) { return op.Operator() == "!"; },
                &LogicalNotOperation),
      // Do all multiplication or division.
      BindFront(&SubInfixBinaryOpMeta,
                [](const Token& op) {
                  return op.Operator() == "*" || op.Operator() == "/";
                },
                &MultiplicativeOperation),
      // Do all addition or subtraction.
      BindFront(&SubInfixBinaryOpMeta,
                [](const Token& op) {
                  return op.Operator() == "+" || op.Operator() == "-";
                },
                &AdditiveOperation),
      // Do relational comparisons.
      BindFront(&SubInfixBinaryOpMeta, &IsRelationalOp, &ComparisonOperation),
      // Do equality comparisons.
      BindFront(&SubInfixBinaryOpMeta, &IsEqualityOp, &ComparisonOperation),
      // Do logical AND.
      BindFront(&SubInfixBinaryOpMeta,
                [](const Token& op) { return op.Operator() == "&&"; },
                &LogicalAndOperation),
      // Do logical OR.
      BindFront(&SubInfixBinaryOpMeta,
                [](const Token& op) { return op.Operator() == "||"; },
                &LogicalOrOperation),
  };
  return *kSubstituteMethods;
}

// Substitute the tokenized 'expr' repeatedly until an error has
// occurred or 'expr' IsValueSequence().
// Returns false if an error occurred.
// Note that expression must be a pure expression without side effects.
// This method implements a simple expression evaluator through substitution on
// a tokenized form of the expression.
// Each SubstitueMethod is called in substitution order on the expression and
// must guarantee that applying it more than once on the 'expr' has no effect.
// Terminate substitution early when an error occurs.
bool SubstituteUntilValue(const Json::Value& store, const Runtime* runtime,
                          FunctionDispatcher* dispatcher,
                          std::list<Token>* expr) {
  // This list of substitutions defines the substitution order.
  const auto& substitute_methods = GetSubstituteMethods();

  bool is_error = false;
  for (const SubstituteMethod& method : substitute_methods) {
    if (method(store, runtime, dispatcher, expr, &is_error)) {
      DVLOG(1) << "expr: " << absl::StrJoin(ListDebugStrings(*expr), " ");
    }
    if (is_error) {
      break;
    }
  }

  if (!is_error && IsValueSequence(expr->begin(), expr->end())) {
    return true;
  }
  DVLOG(1) << "Eval error, remaining tokens: "
           << absl::StrJoin(ListDebugStrings(*expr), " ");
  return false;
}

// Convert a list of string tokens to a list of Tokens. Returns false if an
// error occurred during conversion.
bool ConvertTokens(const Json::Value& store,
                   const FunctionDispatcher& dispatcher,
                   const std::list<string>& str_tokens,
                   std::list<Token>* tokens) {
  for (const string& str : str_tokens) {
    bool is_error = false;
    tokens->push_back(Token::Create(store, dispatcher, str, &is_error));
    if (is_error) {
      DVLOG(1) << "Token creation failed for token: " << str;
      return false;
    }
  }
  return true;
}

// Converts a string expression into the internal format and stores the result
// in 'result'. Returns true if evaluation succeeded.
bool ProcessExpression(const Json::Value& store, const Runtime* runtime,
                       FunctionDispatcher* dispatcher, string expression,
                       Token* result) {
  absl::StripAsciiWhitespace(&expression);
  if (expression.empty()) {
    return false;
  }

  bool is_error = false;
  *result = Token::Create(store, *dispatcher, expression, &is_error);
  if (!is_error && result->IsValue()) {
    DVLOG(1) << "expression is value: " << expression;
    return true;
  }

  std::list<string> string_tokens;
  internal::TokenizeExpression(expression, &string_tokens);
  // Do preprocessing on the string tokens.
  PresubstituteStringTokens(store, &string_tokens);

  // Convert string tokens to Tokens.
  std::list<Token> tokens;
  if (!ConvertTokens(store, *dispatcher, string_tokens, &tokens)) {
    DVLOG(1) << "Token conversion failure for expression: " << expression;
    return false;
  }

  if (!SubstituteUntilValue(store, runtime, dispatcher, &tokens)) {
    return false;
  }
  // Only allow value sequences that are of size 1.
  if (tokens.size() != 1) {
    return false;
  }
  // Store result.
  tokens.front().Swap(result);
  return true;
}

// Validate that 'path' is a valid dot-separated JSON path.
// Note that any string between '.'s is accepted as a valid field name unless
// a subpath from the start of the string is a function name.
bool IsDotSeparatedPath(const FunctionDispatcher& dispatcher,
                        const string& path) {
  if (path.empty()) {
    return false;
  }
  if (path == ".") {
    return true;
  }
  std::vector<absl::string_view> path_tokens = absl::StrSplit(path, ".");
  string path_from_root = string(path_tokens.front());
  if (dispatcher.HasFunction(path_from_root)) {
    return false;
  }
  for (size_t i = 1; i < path_tokens.size(); ++i) {
    if (path_tokens[i].empty()) {
      return false;
    }
    path_from_root += '.';
    path_from_root += string(path_tokens[i]);
    if (dispatcher.HasFunction(path_from_root)) {
      return false;
    }
  }
  return true;
}

// Computes a location expression and destructively modifies the store to
// create the evaluated location if the location does not exists or is null.
// Returns false if an error occurred. Otherwise stores the address of the
// newly created store Json::Value in 'location'.
bool ProcessLocationExpression(Json::Value* store, const Runtime* runtime,
                               FunctionDispatcher* dispatcher,
                               string expression, Json::Value** location) {
  absl::StripAsciiWhitespace(&expression);
  if (expression.empty()) {
    return false;
  }

  std::list<string> string_tokens;
  internal::TokenizeExpression(expression, &string_tokens);
  // Do preprocessing on the string tokens.
  PresubstituteStringTokens(*store, &string_tokens);

  // Validate that the first token is a path.
  // Also check that the token is not a function.
  if (!IsDotSeparatedPath(*dispatcher, string_tokens.front())) {
    return false;
  }
  // Additionally convert leading '.' paths into element accesses so that
  // they can be validated step by step that the Json::Value is of the correct
  // type. E.g. if 'arr' is an array, we need to ensure 'arr.foo' returns
  // false and not assert-fail when Json::Path::make() is called.
  std::vector<absl::string_view> path_tokens =
      absl::StrSplit(string_tokens.front(), ".", absl::SkipEmpty());
  // Insert right after the first token.
  auto it = ++string_tokens.begin();
  for (::std::size_t i = 1; i < path_tokens.size(); ++i) {
    // Convert to element access.
    string_tokens.insert(it, "[");
    string_tokens.insert(it, Quote(string(path_tokens[i])));
    string_tokens.insert(it, "]");
  }
  // Replace the first string_token with the first path_token.
  string_tokens.front() = string(path_tokens.front());

  // Create the root if needed.
  Json::Path json_root_path(string_tokens.front());
  // Flag used to track if a new location was created or not.
  bool is_new_location =
      UndefinedJSON() == json_root_path.resolve(*store, UndefinedJSON());
  Json::Value* new_loc = &json_root_path.make(*store);

  // Convert string tokens to Tokens.
  std::list<Token> tokens;
  if (!ConvertTokens(*store, *dispatcher, string_tokens, &tokens)) {
    DVLOG(1) << "Token conversion failure for expression: " << expression;
    return false;
  }

  // Process all sub-expressions in the parentheses.
  bool is_error = false;
  SubstituteParentheses(*store, runtime, dispatcher, &tokens, &is_error);
  // Token elements must be of the form "root[key1][key2]...[keyN]".
  if (is_error || (tokens.size() - 1) % 3 != 0) {
    return false;
  }

  // Create the subpaths under the root object.
  // Do validation in the process.
  tokens.pop_front();
  for (auto it = tokens.begin(), it_end = tokens.end(); it != it_end; ++it) {
    // Do validation.
    if (!it->IsOperator() || it->Operator() != "[") {
      return false;
    }
    ++it;
    if (it == it_end || !it->IsValue()) {
      return false;
    }
    const Token& field_token = *it;
    ++it;
    if (it == it_end || !it->IsOperator() || it->Operator() != "]") {
      return false;
    }
    // Create subpath locations.
    if (field_token.Value().isString()) {
      // Automatically create object if the location is new.
      if (is_new_location) {
        *new_loc = Json::Value(Json::objectValue);
      }
      if (!new_loc->isObject()) {
        LOG(INFO) << "Object element access failed on non-object: "
                  << ValueToString(*new_loc, true);
        return false;
      }
      string key = field_token.Value().asString();
      is_new_location = !new_loc->isMember(key);
      new_loc = &(*new_loc)[key];
    } else if (field_token.Value().isIntegral()) {
      const int index = field_token.Value().asInt();
      if (index < 0) {
        LOG(INFO) << "Array index out of bounds: " << field_token.DebugString()
                  << ", from expression: " << expression;
        return false;
      }
      // Automatically create new array if location is new.
      if (is_new_location) {
        *new_loc = Json::Value(Json::arrayValue);
      }
      if (!new_loc->isArray()) {
        LOG(INFO) << "Array element access failed on non-array: "
                  << ValueToString(*new_loc, true);
        return false;
      }
      is_new_location = static_cast<::std::size_t>(index) >= new_loc->size();
      new_loc = &(*new_loc)[index];
    } else {
      LOG(INFO) << "Field is not an index or a string: "
                << field_token.DebugString();
      return false;
    }
  }  // for each token group
  *location = new_loc;
  return true;
}

}  // namespace

LightWeightDatamodel::LightWeightDatamodel(FunctionDispatcher* dispatcher)
    : dispatcher_(dispatcher) {}

// static
std::unique_ptr<LightWeightDatamodel> LightWeightDatamodel::Create(
    FunctionDispatcher* dispatcher) {
  RETURN_NULL_IF_MSG(dispatcher == nullptr, "Dispatcher should not be null.");
  return ::absl::WrapUnique(new LightWeightDatamodel(dispatcher));
}

// static
std::unique_ptr<LightWeightDatamodel> LightWeightDatamodel::Create(
    const string& serialized_data, FunctionDispatcher* dispatcher) {
  auto datamodel = LightWeightDatamodel::Create(dispatcher);
  if (!datamodel->ParseFromString(serialized_data)) {
    return nullptr;
  }
  return datamodel;
}

// override
bool LightWeightDatamodel::IsDefined(const string& location) const {
  Token token;
  if (!ProcessExpression(store_, GetRuntime(), dispatcher_, location, &token)) {
    return false;
  }
  return token.IsReference();
}

// override
bool LightWeightDatamodel::Declare(const string& location) {
  if (IsDefined(location) || dispatcher_->HasFunction(location)) {
    return false;
  }
  return DeclareAndAssignJson(location, Json::Value());  // Assigns null.
}

// override
bool LightWeightDatamodel::AssignExpression(const string& location,
                                            const string& expr) {
  Json::Value value;  // null
  if (!expr.empty() && !EvaluateJsonExpression(expr, &value)) {
    LOG(INFO) << "AssignExpression: error evaluating expression: " << expr;
    return false;
  }
  return AssignJson(location, value);
}

// override
bool LightWeightDatamodel::AssignString(const string& location,
                                        const string& str) {
  return AssignExpression(location, Quote(str));
}

// override
bool LightWeightDatamodel::EvaluateBooleanExpression(const string& expr,
                                                     bool* result) const {
  Token token;
  if (!ProcessExpression(store_, GetRuntime(), dispatcher_, expr, &token)) {
    return false;
  }
  *result = token.ToBool();
  return true;
}

// override
bool LightWeightDatamodel::EvaluateStringExpression(const string& expr,
                                                    string* result) const {
  Token token;
  if (!ProcessExpression(store_, GetRuntime(), dispatcher_, expr, &token)) {
    return false;
  }
  *result = ValueToString(token.Value());
  return true;
}

// override
bool LightWeightDatamodel::EvaluateExpression(const string& expr,
                                              string* result) const {
  Token token;
  if (!ProcessExpression(store_, GetRuntime(), dispatcher_, expr, &token)) {
    return false;
  }
  *result = ValueToString(token.Value(), true);
  return true;
}

// override
string LightWeightDatamodel::EncodeParameters(
    const std::map<string, string>& parameters) const {
  return MakeJSONFromStringMap(parameters);
}

bool LightWeightDatamodel::IsAssignable(const string& location) const {
  if (IsDefined(location)) {
    return true;
  }
  std::list<string> string_tokens;
  internal::TokenizeExpression(location, &string_tokens);
  // Do preprocessing on the string tokens.
  PresubstituteStringTokens(store_, &string_tokens);

  bool is_error = false;

  // Single value, this is a path type expression that is undefined.
  if (string_tokens.size() == 1) {
    // Check if the parent path is an object.
    Token token = Token::Create(store_, *dispatcher_,
                                string_tokens.front().substr(
                                    0, string_tokens.front().find_last_of('.')),
                                &is_error);
    return !is_error && token.IsReference() && token.Value().isObject();
  }
  // Convert string tokens to Tokens.
  std::list<Token> tokens;
  if (!ConvertTokens(store_, *dispatcher_, string_tokens, &tokens)) {
    return false;
  }
  // Process all sub-expressions in the parentheses.
  SubstituteParentheses(store_, GetRuntime(), dispatcher_, &tokens, &is_error);
  // Token elements must be of the form "root[key1][key2]...[keyN]".
  if (is_error || (tokens.size() - 1) % 3 != 0) {
    return false;
  }
  // Create a new expression list excluding the last access operation (last 3
  // tokens). This 'parent' expression must evaluate to a reference in the
  // store.
  auto it = tokens.begin();
  std::advance(it, tokens.size() - 3);
  std::list<Token> parent_tokens;
  parent_tokens.splice(parent_tokens.end(), tokens, tokens.begin(), it);
  if (!SubstituteUntilValue(store_, GetRuntime(), dispatcher_,
                            &parent_tokens)) {
    return false;
  }
  RETURN_FALSE_IF(parent_tokens.empty());
  if (!parent_tokens.front().IsReference()) {
    return false;
  }
  const Json::Value& parent_value = parent_tokens.front().Value();
  if (!parent_value.isArray() && !parent_value.isObject()) {
    return false;
  }
  // Check the type of the access key and the parent reference.
  it = ++tokens.begin();
  RETURN_FALSE_IF_MSG(!it->IsValue(), it->DebugString());
  // Array access must have integral operand.
  if (parent_value.isArray() && !it->Value().isIntegral()) {
    return false;
  }
  // Object access requires string operand.
  if (parent_value.isObject() && !it->Value().isString()) {
    return false;
  }
  return true;
}

// override
string LightWeightDatamodel::DebugString() const {
  Json::StyledStreamWriter json_writer("  ");
  std::stringstream sstream;
  json_writer.write(sstream, store_);
  return sstream.str();
}

// override
string LightWeightDatamodel::SerializeAsString() const {
  Json::FastWriter json_writer;
  return json_writer.write(store_);
}

// override
bool LightWeightDatamodel::ParseFromString(const string& data) {
  Json::Reader reader;
  const bool success = reader.parse(data, store_, false /* collectComments */);
  LOG_IF(ERROR, !success) << "Failed in reading as Json::Value. Error: "
                          << reader.getFormattedErrorMessages()
                          << "\nValue : " << data;
  return success;
}

// override
void LightWeightDatamodel::Clear() { store_.clear(); }

// override
std::unique_ptr<Datamodel> LightWeightDatamodel::Clone() const {
  auto lwdm = absl::WrapUnique(new LightWeightDatamodel(this->dispatcher_));
  lwdm->store_ = this->store_;
  lwdm->runtime_ = this->runtime_;
  return lwdm;
}

bool LightWeightDatamodel::EvaluateJsonExpression(const string& expr,
                                                  Json::Value* result) const {
  Token token;
  if (!ProcessExpression(store_, GetRuntime(), dispatcher_, expr, &token)) {
    return false;
  }
  *result = token.Value();
  return true;
}

// override
std::unique_ptr<Iterator> LightWeightDatamodel::EvaluateIterator(
    const string& location) const {
  // Only arrays are supported.
  Token token;
  if (!ProcessExpression(store_, GetRuntime(), dispatcher_, location, &token) ||
      !token.IsValue() || !token.Value().isArray()) {
    LOG(INFO) << "EvaluateIterator: error evaluating location: " << location
              << ", resulting token: " << token.DebugString();
    return nullptr;
  }
  // If it is a location in the store, do not copy, use reference.
  if (token.IsReference()) {
    return absl::make_unique<ArrayReferenceIterator>(token.Value());
  }
  // 'token' has a value. 'token.MutableValue()' is not NULL. Move value into
  // the iterator.
  return absl::make_unique<ArrayValueIterator>(token.MutableValue());
}

bool LightWeightDatamodel::AssignJson(const string& location,
                                      const Json::Value& value) {
  if (!IsAssignable(location)) {
    VLOG(1) << "AssignJson: location is not assignable: " << location;
    return false;
  }

  return DeclareAndAssignJson(location, value);
}

bool LightWeightDatamodel::DeclareAndAssignJson(const string& location,
                                                const Json::Value& value) {
  Json::Value* new_loc = nullptr;
  // Evaluate the location expression and destructively create new paths
  // in the store.
  if (!ProcessLocationExpression(&store_, GetRuntime(), dispatcher_, location,
                                 &new_loc)) {
    LOG(INFO) << "DeclareAndAssignJson: error evaluating location: "
              << location;
    return false;
  }
  DVLOG(1) << "DeclareAndAssignJson: Storing: " << location << " = "
           << Json::FastWriter().write(value);
  *new_loc = value;
  return true;
}

namespace internal {

void TokenizeExpression(const string& expr, std::list<string>* tokens) {
  // Marks the start of an operand token (i.e. a non-operator token).
  ::std::size_t token_start = 0;
  // The type of string quote character.
  char str_quote_type = 0;
  bool in_string = false;
  string operand;
  for (::std::size_t i = 0; i < expr.size(); ++i) {
    // Set the string flag if we enter or exit a string.
    if (expr[i] == '"' || expr[i] == '\'') {
      if (in_string) {
        // No longer in a string if non-escaped 'str_quote_type' met.
        in_string = (expr[i - 1] == '\\' || expr[i] != str_quote_type);
      } else {
        in_string = true;
        str_quote_type = expr[i];
      }
    }
    if (in_string) {
      continue;
    }
    // Expression from i-th position onwards.
    absl::string_view subexpr(expr.data() + i);
    // Search for the longest operator (or delimiter) match.
    absl::string_view matched_op;
    for (const auto& op : kOperators) {
      if (!::absl::StartsWith(subexpr, op)) {
        continue;
      }
      if (strlen(op) > matched_op.size()) {
        matched_op = op;
      }
    }
    // No operator found.
    if (matched_op.empty()) {
      continue;
    }
    // Write out the current operand token.
    if (token_start < i) {
      operand = expr.substr(token_start, i - token_start);
      absl::StripAsciiWhitespace(&operand);
      if (!operand.empty()) {
        tokens->push_back(operand);
      }
    }
    // Write the operator.
    tokens->push_back(string(matched_op));
    // Next operand comes after operator.
    token_start = i + matched_op.size();
    // Move i forward so that we do not match another operator
    // that is a substring of the previous operator.
    i += matched_op.size() - 1;
  }
  // Store the final operand.
  if (token_start < expr.size()) {
    operand = expr.substr(token_start);
    absl::StripAsciiWhitespace(&operand);
    if (!operand.empty()) {
      tokens->push_back(operand);
    }
  }
}

}  // namespace internal

}  // namespace state_chart
