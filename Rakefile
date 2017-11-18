task default: %w[clean]

desc "clean ignore file"
task :clean do
  sh "rm -rf Makefile CMakeCache.txt cmake_install.cmake CTestTestfile.cmake CMakeFiles cmake-build-debug"
  sh "rm -rf *.a *.dylib *.so *.cbp *.log vgcore.* lua/src/*.o lua/src/lua lua/src/luac"
  sh "rm -rf libyaml/include/config.h libyaml.* ip2socks ip2socks.dSYM liblua.* lua.cbp"
end
