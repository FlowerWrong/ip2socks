task default: %w[clean]

task :clean do
  sh "rm -f libyaml/include/config.h Makefile CMakeCache.txt cmake_install.cmake CTestTestfile.cmake *.a *.dylib *.so ip2socks *.cbp *.log vgcore.*"
  sh "rm -rf CMakeFiles cmake-build-debug ip2socks.dSYM libyaml.*"
end
