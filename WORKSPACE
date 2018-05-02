workspace(name = "com_google_statechart")

# Using protobuf version 3.5.1
http_archive(
    name = "com_google_protobuf",
    strip_prefix = "protobuf-3.5.1",
    urls = ["https://github.com/google/protobuf/archive/v3.5.1.tar.gz"],
)

http_archive(
    name = "com_google_protobuf_cc",
    strip_prefix = "protobuf-3.5.1",
    urls = ["https://github.com/google/protobuf/archive/v3.5.1.tar.gz"],
)

# GoogleTest/GoogleMock framework. Used by most unit-tests.
http_archive(
     name = "com_google_googletest",
     urls = ["https://github.com/google/googletest/archive/master.zip"],
     strip_prefix = "googletest-master",
)

# We depend on Abseil.
http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-master",
    urls = ["https://github.com/abseil/abseil-cpp/archive/master.zip"],
)

# Abseil depends on CCTZ (Time-zone framework).
http_archive(
    name = "com_googlesource_code_cctz",
    strip_prefix = "cctz-master",
    urls = ["https://github.com/google/cctz/archive/master.zip"],
)

http_archive(
    name = "com_googlesource_code_re2",
    urls = ["https://github.com/google/re2/archive/master.zip"],
    strip_prefix = "re2-master",
)

new_http_archive(
    name = "jsoncpp_git",
    urls = ["https://github.com/open-source-parsers/jsoncpp/archive/1.7.6.tar.gz"],
    strip_prefix = "jsoncpp-1.7.6",
    build_file = "//third_party:jsoncpp.BUILD",
)

http_archive(
    name = "com_github_gflags_gflags",
    urls = [ "https://github.com/gflags/gflags/archive/master.zip" ],
    strip_prefix = "gflags-master"
)
# 
http_archive(
    name = "com_google_glog",
    urls = [ "https://github.com/google/glog/archive/master.zip" ],
    strip_prefix = "glog-master"
)

# gflags needed by glog
# http_archive(
#     name = "com_github_gflags_gflags",
#     sha256 = "6e16c8bc91b1310a44f3965e616383dbda48f83e8c1eaa2370a215057b00cabe",
#     strip_prefix = "gflags-77592648e3f3be87d6c7123eb81cbad75f9aef5a",
#     urls = [
#         "https://mirror.bazel.build/github.com/gflags/gflags/archive/77592648e3f3be87d6c7123eb81cbad75f9aef5a.tar.gz",
#         "https://github.com/gflags/gflags/archive/77592648e3f3be87d6c7123eb81cbad75f9aef5a.tar.gz",
#     ],
# )

# glog
# http_archive(
#     name = "com_google_glog",
#     sha256 = "1ee310e5d0a19b9d584a855000434bb724aa744745d5b8ab1855c85bff8a8e21",
#     strip_prefix = "glog-028d37889a1e80e8a07da1b8945ac706259e5fd8",
#     urls = [
#         "https://mirror.bazel.build/github.com/google/glog/archive/028d37889a1e80e8a07da1b8945ac706259e5fd8.tar.gz",
#         "https://github.com/google/glog/archive/028d37889a1e80e8a07da1b8945ac706259e5fd8.tar.gz",
#     ],
# )