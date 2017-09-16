require_relative 'init'

require 'socket'
require 'resolv'
require_relative 'dns/extensions/resolv'
require_relative 'dns/message'

require 'awesome_print'


class DNSClient
  attr_accessor :server, :logger, :response, :query

  def initialize(server, logger)
    @server = server
    @logger = logger
  end

  def handler
    input_data, remote_address = @server.recvfrom_nonblock(1460)
    response = process_query(input_data, remote_address: remote_address)
    output_data = response.encode
    @server.send output_data, 0, remote_address[3], remote_address[1]
  end

  private

  def fully_qualified_name(name)
    # If we are passed an existing deconstructed name:
    if Resolv::DNS::Name === name
      if name.absolute?
        name
      else
        name.with_origin(".")
      end
    end

    # ..else if we have a string, we need to do some basic processing:
    if name.end_with? '.'
      Resolv::DNS::Name.create(name)
    else
      Resolv::DNS::Name.create(name).with_origin(".")
    end
  end

  # Provides the next sequence identification number which is used to keep track of DNS messages.
  def next_id!
    # Using sequential numbers for the query ID is generally a bad thing because over UDP they can be spoofed. 16-bits isn't hard to guess either, but over UDP we also use a random port, so this makes effectively 32-bits of entropy to guess per request.
    SecureRandom.random_number(2**16)
  end

  def fail!(rcode, response)
    if rcode.kind_of? Symbol
      response.rcode = Resolv::DNS::RCode.const_get(rcode)
    else
      response.rcode = rcode.to_i
    end
    response
  end

  def build_basic_dns_res(query)
    # Setup response
    response = Resolv::DNS::Message::new(query.id)
    response.qr = 1 # 0 = Query, 1 = Response
    response.opcode = query.opcode # Type of Query; copy from query
    response.aa = 1 # Is this an authoritative response: 0 = No, 1 = Yes
    response.rd = query.rd # Is Recursion Desired, copied from query
    response.ra = 0 # Does name server support recursion: 0 = No, 1 = Yes
    response.rcode = 0 # Response code: 0 = No errors

    response
  end

  def process_query(data, options)
    @logger.debug "<> Receiving incoming query (#{data.bytesize} bytes) to #{self.class.name}...#{options}"

    query = nil
    begin
      query = DNS::decode_message(data)
    rescue StandardError => error
      @logger.error "<> Error processing request: #{error.inspect}!"
      return error_response(query)
    end

    response = build_basic_dns_res(query)
    begin
      query.question.each do |question, resource_class|
        begin
          question = question.without_origin('.')

          @logger.debug {"<#{query.id}> Processing question #{question} #{resource_class}"}

          if query.rd
            message = Resolv::DNS::Message.new(next_id!)
            message.rd = 1
            message.add_question fully_qualified_name(question.to_s), resource_class

            upstream = UDPSocket.new
            upstream.send(message.encode, 0, '114.114.114.114', 53)
            upstream_input_data, _ = upstream.recvfrom(1460)

            res_msg = DNS::decode_message(upstream_input_data)

            if res_msg
              # Recursion is available and is being used:
              # See issue #26 for more details.
              res_msg.ra = 1
              response.merge!(res_msg)
            else
              response = fail!(:ServFail, response)
            end
          else
            response = fail!(:Refused, response)
          end
        rescue Resolv::DNS::OriginError
          # This is triggered if the question is not part of the specified @origin:
          @logger.debug {"<#{query.id}> Skipping question #{question} #{resource_class} because #{$!}"}
        end
      end
    rescue StandardError => error
      @logger.error "<#{query.id}> Exception thrown while processing! #{error.inspect}!"
      response.rcode = Resolv::DNS::RCode::ServFail
    end
    return response
  end

  def error_response(query = nil, code = Resolv::DNS::RCode::ServFail)
    # Encoding may fail, so we need to handle this particular case:
    server_failure = Resolv::DNS::Message::new(query ? query.id : 0)

    server_failure.qr = 1
    server_failure.opcode = query ? query.opcode : 0
    server_failure.aa = 1
    server_failure.rd = 0
    server_failure.ra = 0
    server_failure.rcode = code

    # We can't do anything at this point...
    server_failure
  end
end
