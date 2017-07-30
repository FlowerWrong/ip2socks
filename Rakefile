task default: %w[clean]

task :clean do
  sh "rm -f libyaml/include/config.h Makefile CMakeCache.txt cmake_install.cmake *.a CTestTestfile.cmake ip2socks uv_socks_client ev_socks_client ev_socks_server *.cbp *.log vgcore.*"
  sh "rm -rf CMakeFiles cmake-build-debug ip2socks.dSYM libyaml.*"
end
