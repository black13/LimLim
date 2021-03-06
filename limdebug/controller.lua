#! /usr/bin/lua
-- RemDebug 1.0
-- Copyright Kepler Project 2005-2007 (http://www.keplerproject.org/remdebug)
--
-- Modified 2011-2012 for LuaIDE project by Simon Mikuda to LimDebug
-- Bachelor project on STU Fiit
-- Project manager: Ing. Michal Kottman
-- Copyright Simon Mikuda STU Fiit 2012
--

print "LimDebug controller"

local socket = require "socket"

local server = socket.bind("*", 8172)
if server == nil then
  print "Error: Remdebug already running"
  os.exit()
else
  print "Start program you want to debug:"
end

local client = server:accept()

local breakpoints = {}
local watches = {}

client:send("STEP\n")
client:receive()

local breakpoint = client:receive()
local _, _, file, line = string.find(breakpoint, "^202 Paused%s+([%w%p]+)%s+(%d+)$")
if file and line then
  print("Paused:" .. " file " .. file )
else
  local _, _, size = string.find(breakpoint, "^401 Error in Execution (%d+)$")
  if size then
    print("Error in remote application: ")
    print(client:receive(size))
  end
end

local basedir = ""

while true do

  io.write("> ")
  --io.flush()

  -- Read new command

  local line = io.read("*line")
  local _, _, command = string.find(line, "^([a-z]+)")

  --
  --  RUN, STEP, OVER commands
  --
  if command == "run" or command == "step" or command == "over" then
        client:send(string.upper(command) .. "\n")
        client:receive()
        local breakpoint = client:receive()
        if not breakpoint then
          os.exit()
        end
        local _, _, status = string.find(breakpoint, "^(%d+)")
        if status == "202" then
          local _, _, file, line = string.find(breakpoint, "^202 Paused (.+) (%d+)$")
          if file and line then
                print("Paused:"  .. " line " .. line .. " file " .. file)
          end
        elseif status == "203" then
          local _, _, file, line, watch_idx = string.find(breakpoint, "^203 Paused (.+) (%d+) (%d+)$")
          if file and line and watch_idx then
                print("Paused:" .. " line " .. line .. " watch " .. watch_idx .. " file " .. file)
          end
        elseif status == "401" then
          local _, _, size = string.find(breakpoint, "^401 Error in Execution (%d+)$")
          if size then
                print("Error in remote application: ")
                print(client:receive(tonumber(size)))
                os.exit()
          end
        else
          print("Error: Unknown error")
          os.exit()
        end

  --
  -- EXIT command
  --
  elseif command == "exit" then
        client:close()
        os.exit()


  --
  -- SETB command
  --
  elseif command == "setb" then
        local _, _, filename, line = string.find(line, "^.... (.+) (%d+)$")
        if filename and line then
          filename = basedir .. filename
          if not breakpoints[filename] then breakpoints[filename] = {} end
          client:send("SETB " .. filename .. " " .. line .. "\n")
          if client:receive() == "200 OK" then
                breakpoints[filename][line] = true
          else
                print("Error: breakpoint not inserted")
          end
        else
          print("Error: Invalid command")
        end


  --
  -- SETW command
  --
  elseif command == "setw" then
        local _, _, exp = string.find(line, "^[a-z]+%s+(.+)$")
        if exp then
          client:send("SETW " .. exp .. "\n")
          local answer = client:receive()
          local _, _, watch_idx = string.find(answer, "^200 OK (%d+)$")
          if watch_idx then
                watches[watch_idx] = exp
                print("Watch: " .. watch_idx)
          else
                print("Error: Watch expression not inserted")
          end
        else
          print("Error: Invalid command")
        end

  --
  -- DELB command
  --
  elseif command == "delb" then
        local _, _, filename, line = string.find(line, "^.... (.+) (%d+)$")
        if filename and line then
          filename = basedir .. filename
          if not breakpoints[filename] then breakpoints[filename] = {} end
          client:send("DELB " .. filename .. " " .. line .. "\n")
          if client:receive() == "200 OK" then
                breakpoints[filename][line] = nil
          else
                print("Error: breakpoint not removed")
          end
        else
          print("Error: Invalid command")
        end


  --
  -- DELALLB command
  --
  elseif command == "delallb" then
        for filename, breaks in pairs(breakpoints) do
          for line, _ in pairs(breaks) do
                client:send("DELB " .. filename .. " " .. line .. "\n")
                if client:receive() == "200 OK" then
                  breakpoints[filename][line] = nil
                else
                  print("Error: breakpoint at file " .. filename .. " line " .. line .. " not removed")
                end
          end
        end

  --
  -- DELW command
  --
  elseif command == "delw" then
        local _, _, index = string.find(line, "^[a-z]+%s+(%d+)$")
        if index then
          client:send("DELW " .. index .. "\n")
          if client:receive() == "200 OK" then
          watches[index] = nil
          else
                print("Error: watch expression not removed")
          end
        else
          print("Error: Invalid command")
        end
  elseif command == "delallw" then
        for index, exp in pairs(watches) do
          client:send("DELW " .. index .. "\n")
          if client:receive() == "200 OK" then
          watches[index] = nil
          else
                print("Error: watch expression at index " .. index .. " [" .. exp .. "] not removed")
          end
        end

  --
  -- EVAL command
  --
  elseif command == "eval" then
        local _, _, exp = string.find(line, "^%a+%s+(.+)$")
        if exp then
          client:send("EVAL return " .. exp .. ", 1\n")
          local line = client:receive()
		  local _, _, status, len = string.find(line, "^(%d+)[a-zA-Z ]+%s*(%d+)$")
          if status == "200" then
                len = tonumber(len)
                local res = client:receive(len)
                print("Evaluate: " .. res)
          elseif status == "401" then
                len = tonumber(len)
                local res = client:receive(len)
                print("Error: Invalid expression:" .. res)
          else
                print("Error: Unknown error")
          end

        else
          print("Error: Invalid command")
        end

  --
  -- TABLE command
  --
  elseif command == "table" then
        local _, _, exp = string.find(line, "^[a-z]+%s+(.+)$")
        if exp then
          client:send("TABLE " .. exp .. "\n")
          local line = client:receive()
          local _, _, status, len = string.find(line, "^(%d+)[a-zA-Z ]+%s*(%d+)$")
          if status == "200" then
                len = tonumber(len)
                local res = client:receive(len)
                io.write("Table: " .. res)
          elseif status == "401" then
                len = tonumber(len)
                local res = client:receive(len)
                print("Error: Invalid expression:" .. res)
          else
                print("Error: Unknown error")
          end

        else
          print("Error: Invalid command")
        end
  --
  -- TRACEBACK command
  --
  elseif command == "traceback" then
      client:send("TRACEBACK\n")
      local line = client:receive()
      local _, _, status, len = string.find(line, "^(%d+)[a-zA-Z ]+%s*(%d+)$")
      if status == "200" then
            len = tonumber(len)
            local res = client:receive(len)
            print(res)
      elseif status == "401" then
            len = tonumber(len)
            local res = client:receive(len)
            print("Error: Invalid expression:" .. res)
      else
            print("Error: Unknown error")
      end

  --
  -- LOCAL command
  --
  elseif command == "local" then
      client:send("LOCAL\n")
      local line = client:receive()
      local _, _, status, len = string.find(line, "^(%d+)[a-zA-Z ]+%s*(%d+)$")
      if status == "200" then
            len = tonumber(len)
            local res = client:receive(len)
            io.write("Local: " .. res)
      elseif status == "401" then
            len = tonumber(len)
            local res = client:receive(len)
            print("Error: Invalid expression:" .. res)
      else
            print("Error: Unknown error")
      end

  --
  -- GLOBAL command
  --
  elseif command == "global" then
      client:send("GLOBAL\n")
      local line = client:receive()
      local _, _, status, len = string.find(line, "^(%d+)[a-zA-Z ]+%s*(%d+)$")
      if status == "200" then
            len = tonumber(len)
            local res = client:receive(len)
            io.write("Global: " .. res)
      elseif status == "401" then
            len = tonumber(len)
            local res = client:receive(len)
            print("Error: Invalid expression:" .. res)
      else
            print("Error: Unknown error")
      end

  --
  -- EXEC command
  --
  elseif command == "exec" then
        local _, _, exp = string.find(line, "^[a-z]+%s+(.+)$")
        if exp then
          client:send("EXEC " .. exp .. "\n")
          local line = client:receive()
          local _, _, status, len = string.find(line, "^(%d+)[%s%w]+ (%d+)$")
          if status == "200" then
          elseif status == "401" then
                len = tonumber(len)
                local res = client:receive(len)
                print("Error: Invalid expression" .. res)
          else
                print("Error: Unknown error")
          end
        else
          print("Error: Invalid command")
        end

  --
  -- LISTB command
  --
  elseif command == "listb" then
        for k, v in pairs(breakpoints) do
          io.write(k .. ": ")
          for k, v in pairs(v) do
                io.write(k .. " ")
          end
          io.write("\n")
        end

  --
  -- LISTW command
  --
  elseif command == "listw" then
        for i, v in pairs(watches) do
          print("Watch exp. " .. i .. ": " .. v)
        end

  --
  -- BASEDIR command
  --
  elseif command == "basedir" then
        local _, _, dir = string.find(line, "^[a-z]+%s+(.+)$")
        if dir then
          dir = string.gsub(dir, "\\", "/")
          if not string.find(dir, "/$") then dir = dir .. "/" end
          basedir = dir
          print("New base directory is " .. basedir)
        else
          print(basedir)
        end

  --
  -- HELP command
  --
  elseif command == "help" then
        print("setb <file> <line>    -- sets a breakpoint")
        print("delb <file> <line>    -- removes a breakpoint")
        print("delallb               -- removes all breakpoints")
        print("setw <exp>            -- adds a new watch expression")
        print("delw <index>          -- removes the watch expression at index")
        print("delallw               -- removes all watch expressions")
        print("run                   -- run until next breakpoint")
        print("step                  -- run until next line, stepping into function calls")
        print("over                  -- run until next line, stepping over function calls")
        print("listb                 -- lists breakpoints")
        print("listw                 -- lists watch expressions")
        print("eval <exp>            -- evaluates expression on the current context and returns its value")
        print("exec <stmt>           -- executes statement on the current context")
        print("table <name>	         -- ")
        print("traceback             -- ")
        print("basedir [<path>]      -- sets the base path of the remote application, or shows the current one")
        print("exit                  -- exits debugger")
  else
        local _, _, spaces = string.find(line, "^(%s*)$")
        if not spaces then
          print("Error: Invalid command")
        end
  end

end
