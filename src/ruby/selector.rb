
module Rbev
  class Selector
    def initialize()
      @selectables = {}
      @lock = Mutex.new

      # Other threads can wake up a selector
      @wakeup, @waker = IO.pipe
      @closed = false
    end
  end
end
