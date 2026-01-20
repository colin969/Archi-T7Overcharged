apclient = {
  source = path.join(dependencies.basePath),
}

function apclient.import()
  apclient.includes()
end

function apclient.includes()
  includedirs {
    path.join(apclient.source, "apclientpp"),
    path.join(apclient.source, "json/include"),
    path.join(apclient.source, "asio/include"),
    path.join(apclient.source, "valijson/include"),
    path.join(apclient.source, "wswrap/include"),
    path.join(apclient.source, "websocketpp"),
  }

  defines {
    "AP_NO_SCHEMA",
    "ASIO_STANDALONE",
    "_WEBSOCKETPP_CPP11_RANDOM_DEVICE_",
    "_WEBSOCKETPP_CPP11_TYPE_TRAITS_",
  }

  filter "system:windows"
  defines {
    "_WIN32_WINNT=0x0600",  -- required for ASIO
  }
  includedirs {
    "vcpkg_installed/vcpkg/pkgs/openssl_x64-windows-static/include",
    "vcpkg_installed/vcpkg/pkgs/zlib_x64-windows-static/include",
  }
  libdirs {
    "vcpkg_installed/vcpkg/pkgs/openssl_x64-windows-static/lib",
    "vcpkg_installed/vcpkg/pkgs/zlib_x64-windows-static/lib",
  }
  links {
    "libssl",
    "libcrypto",
    "zlib",
  }

  filter "system:not windows"
  links {
    "ssl",
    "crypto",
    "z"
  }

  filter "action:vs*"
  buildoptions { 
    "/bigobj",
    "/Zc:__cplusplus"
  }

  filter {}
end

function apclient.project()
  -- No project needed for header-only library
end

table.insert(dependencies, apclient)