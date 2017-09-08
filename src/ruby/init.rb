bundler_gem_path = File.expand_path("../../../build/ruby/gems/vendor/bundle", __FILE__)

Dir.glob(File.join(bundler_gem_path, "ruby/**/gems/**")).each do |gem_path|
  $LOAD_PATH.push(File.join(gem_path, 'lib'))
end
