task default: %w[clean]

desc "clean ignore file"
task :clean do
  sh "rm -rf Makefile CMakeCache.txt cmake_install.cmake CTestTestfile.cmake CMakeFiles cmake-build-debug"
  sh "rm -rf *.a *.dylib *.so *.cbp *.log vgcore.*"
  sh "rm -rf libyaml/include/config.h libyaml.* ip2socks ip2socks.dSYM"
end

desc "install gems"
task :gem_install do
  install_dir = './build/ruby/gems/vendor/bundle/ruby/2.4.0'
  gem_bin = './build/ruby/bin/gem'
  ruby_bin = './build/ruby/bin/ruby'
  sh "#{ruby_bin} -v"
  sh "#{gem_bin} -v"
  ['http_parser.rb', 'activesupport', 'awesome_print', 'dnsruby', 'packetfu', 'packetgen', 'ipaddress'].each do |gem|
    sh "#{gem_bin} install --install-dir #{install_dir} #{gem}"
  end
end
