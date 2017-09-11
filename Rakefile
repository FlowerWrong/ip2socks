task default: %w[clean]

desc "clean ignore file"
task :clean do
  sh "rm -f libyaml/include/config.h Makefile CMakeCache.txt cmake_install.cmake CTestTestfile.cmake *.a *.dylib *.so ip2socks *.cbp *.log vgcore.*"
  sh "rm -rf CMakeFiles cmake-build-debug ip2socks.dSYM libyaml.*"
end

desc "install http_parser.rb gem"
task :http_parser do
  sh 'git clone git@github.com:tmm1/http_parser.rb.git'
  sh 'cd http_parser.rb & ../build/ruby/bin/gem build http_parser.rb.gemspe'
  sh '../build/ruby/bin/gem install --install-dir ../build/ruby/gems/vendor/bundle/ruby/2.4.0 http_parser.rb-0.6.0.gem'
  sh 'rm -rf http_parser.rb'
end
