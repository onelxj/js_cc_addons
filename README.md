# js_cc_addons

While we are using C++ addons, C++ code needs to have node dependepncies,
`nodejs_register_toolchains` function downloads actual nodejs sources, but
it doesn't ship a package to access the sources. We can either have a modified
submodule in the repo or just apply a patch, which adds `cc_library` rule at
nodejs directory to allow access the library and use it in bazel environment.