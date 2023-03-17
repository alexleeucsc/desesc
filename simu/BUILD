# This file is distributed under the BSD 3-Clause License. See LICENSE for details.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//tools:copt_default.bzl", "COPTS")

cc_library(
    name = "simu",
    srcs = glob(
        ["*.cpp"],
        exclude = ["*_test*.cpp", "*_bench*.cpp"],
    ),
    hdrs = glob(["*.hpp"]),
    copts = COPTS,
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        "//core:core",
        "//emul:emul",
    ]
)

cc_test(
    name = "store_buffer_test",
    srcs = [
        "store_buffer_test.cpp",
    ],
    deps = [
        ":simu",
        "@com_google_googletest//:gtest_main",
    ],
)

