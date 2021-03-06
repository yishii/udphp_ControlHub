#!/usr/bin/ruby
# -*- coding: undecided -*-
#
# UDP Hole punching control hub
#
# Coded by Yasuhiro ISHII 2012,2013
#
# This software is distributed under Apache 2.0 license.

require "pp"
require "socket"
require "thread"
require "timeout"

# Mutex for not to access password DB synchronously from multiple threads
$passwd_db_mutex = Mutex.new

# User information database(temporary implementation)
$passwd_db = {
  "user0" => { "pass"=>"pass0","connected"=>false,"ip"=>nil,"connect_to"=>nil },
  "user1" => { "pass"=>"pass1","connected"=>false,"ip"=>nil,"connect_to"=>nil },
  "user2" => { "pass"=>"pass2","connected"=>false,"ip"=>nil,"connect_to"=>nil },
  "user3" => { "pass"=>"pass3","connected"=>false,"ip"=>nil,"connect_to"=>nil },
  "user4" => { "pass"=>"pass4","connected"=>false,"ip"=>nil,"connect_to"=>nil },
}

# TCP Socket port number
server_port = 12800

$timeout_login_sec = 10
$timeout_console_sec = 10

def user_authentication(sock)
  printf("## Authentication start\n")

  user_id = nil
  pass_phrase = nil
  result_auth = false

  begin
    timeout($timeout_login_sec){

      while(user_id == nil || pass_phrase == nil)
        msg = sock.gets
        msg = msg.gsub(/(\r|\n)/,"")
        
        printf("input msg = [%s]\n",msg)
        if msg =~ /USER (.*)/
          user_id = $1
          sock.write("OK\r\n")
          sock.flush
          printf("User ID = [%s]\n",user_id)
        elsif msg =~ /PASS (.*)/
          pass_phrase = $1
          sock.write("OK\r\n")
          sock.flush
        end
      end
  
      result_auth = acdb_check_account(user_id,pass_phrase)

      printf("## Authentication end\n")
      STDOUT.flush

    }
  rescue Timeout::Error
    printf("## Authentication timeout error\n")
    result_auth = false
    user_id = nil
  end

  sleep 0.5
  return result_auth,user_id
end

def hub_main(sock,user_id)

  sock.write("CONNECTED\n")
  
  while true
    begin
      timeout($timeout_console_sec){
        received = sock.gets
        result = proc_hub_main_commands(sock,received,user_id)
        printf("** result = %s\n",result)
        if result == false
          # TODO:later need to fix it.
          raise Timeout::Error
        end
        p $passwd_db
      }
    rescue Timeout::Error
      printf("Timeout occurred\n")
      STDOUT.flush
      break
    end
  end
  
  # for debug
  pp $passwd_db

end

def proc_hub_main_commands(sock,data,own_user_id)

  str = data.gsub(/(\r|\n)/,"")
  str_array = str.split(/ /)
  cmd = str_array[0]
  
  p str_array
  p cmd

  case cmd
  when "QUIT"
    return false
  when "STAT"
    if acdb_get_connected(str_array[1])
      sock.write("1\r\n")
    else
      sock.write("*\r\n")
    end
  when "GETIP"
    if acdb_get_connected(str_array[1])
      sock.write(acdb_get_ip(str_array[1])+"\r\n")
    else
      sock.write("*\r\n")
    end
  when "CONNECTTO"
    printf("Try to connect from %s to %s : ",own_user_id,str_array[1])
    STDOUT.flush

    if acdb_set_connect_to(str_array[1],own_user_id)
      printf("OK\n")
      sock.write("1\r\n")
    else
      printf("FAILED\n")
      sock.write("*\r\n")
    end
  when "POLL"
    result = acdb_get_connect_to(own_user_id)
    if result == nil
      sock.write("*\r\n")
    else
      sock.write(result + "\r\n")
    end
  end

  return true

end

##################################################
# Account database

def acdb_get_connect_to(user_id)
  ret = nil

  $passwd_db_mutex.synchronize {
    if $passwd_db.has_key?(user_id)
      ret = $passwd_db[user_id]["connect_to"]
    end
  }

  return ret
end

def acdb_set_connect_to(user_id,own_user_id)
  ret = false

  printf("acdb_set_connect_to(%s,%s)\n",user_id,own_user_id)
  STDOUT.flush

  if acdb_get_connected(user_id) == true
    if acdb_get_connect_to(user_id) == nil
      $passwd_db_mutex.synchronize {
        $passwd_db[user_id]["connect_to"] = own_user_id
      }
      ret = true
    end
  end

  return ret
end

def acdb_get_connect_to(user_id)
  ret = nil

  printf("acdb_get_connect_to [%s]\n",user_id)

  $passwd_db_mutex.synchronize {
    if $passwd_db.has_key?(user_id)
      ret = $passwd_db[user_id]["connect_to"]
    end
  }

  return ret
end

def acdb_check_account(user_id,pass_phrase)
  ret = false

  printf("acdb_check_account ")

  $passwd_db_mutex.synchronize {
    
    if($passwd_db.has_key?(user_id))
      if($passwd_db[user_id]["pass"] == pass_phrase)
        printf("PASS\n")
        ret = true
      else
        printf("FAIL\n")
      end
    else
      printf("FAIL(wrong username)\n")
    end
  }

  STDOUT.flush

  return ret
end

def acdb_set_connected(user_id,connected)
  ret = false
  
  $passwd_db_mutex.synchronize {
    if $passwd_db.has_key?(user_id)
      $passwd_db[user_id]["connected"] = connected
      ret = true
    end
  }

  return ret
end

def acdb_get_connected(user_id)
  ret = false

  $passwd_db_mutex.synchronize {
    if $passwd_db.has_key?(user_id)
      ret = $passwd_db[user_id]["connected"]
    end
  }

  return ret
end

def acdb_set_ip(user_id,ip)
  ret = false
  
  $passwd_db_mutex.synchronize {
    if $passwd_db.has_key?(user_id)
      $passwd_db[user_id]["ip"] = ip
      ret = true
    end
  }

  printf("acdb_set_ip %s=%s\n",user_id,ip)
  return ret
end

def acdb_get_ip(user_id)
  ret = nil

  printf("acdb_get_ip [%s]\n",user_id)

  $passwd_db_mutex.synchronize {
    if $passwd_db.has_key?(user_id)
      ret = $passwd_db[user_id]["ip"]
    end
  }

  return ret
end

##################################################

gs = TCPServer.open(server_port)
addr = gs.addr
addr.shift
printf("server is on %s\n", addr.join(":"))


while true

  Thread.start(gs.accept) do |s|
    s.setsockopt(Socket::IPPROTO_TCP, Socket::TCP_NODELAY, 1)
    
    user_id = nil

    printf("CONNECT %s\n",s)
    #p (s.addr)
    #p (s.peeraddr)
    peer_ipaddr = s.peeraddr[3]
    printf(" from [%s]\n",peer_ipaddr)

    s.write("CONNECT\r\n")
    s.flush

    auth_result,user_id = user_authentication(s)
    if auth_result == false
      # the user authentication is failed

      printf("User authentication failed!!!!!!")
      sleep 2
      s.close

    elsif acdb_get_connected(user_id) == true
      # the user is already logged in

      s.write("ERROR ALREADY CONNECTED\n")
      s.close

    else

      acdb_set_ip(user_id,peer_ipaddr)
      acdb_set_connected(user_id,true)

      hub_main(s,user_id)

      print(s, " is gone\n")
      s.close
      acdb_set_connected(user_id,false)
      acdb_set_ip(user_id,nil)
      p $passwd_db

    end
  end
end
