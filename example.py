import serial
ser = None

try:
    ser = serial.Serial('COM6', 115200, timeout=2)
    greeting = ser.readline()
    
    # Check we can talk with the device, should respond with PONG
    startmarker = 254
    endmarker = 255
    command = 1

    ser.write( startmarker.to_bytes(1, 'big') )
    ser.write( command.to_bytes(1, 'big') )
    ser.write( endmarker.to_bytes(1, 'big') )

    reply = ser.readline()
    reply_ascii = reply.decode('ascii').rstrip()

    if reply_ascii == "PONG":
        print("Sucessfully connected to device")

        # Set screen resolution for the Absolute Mouse library
        command = 4 # Init mouse
        cmd_arg1 = 2560     # Width
        cmd_arg2 = 1440     # Height
        
        ser.write( startmarker.to_bytes(1, 'big') )
        ser.write( command.to_bytes(1, 'big') )
        ser.write( cmd_arg1.to_bytes(2, 'big') )
        ser.write( cmd_arg2.to_bytes(2, 'big') )
        ser.write( endmarker.to_bytes(1, 'big') )

        print(f"Set mouse resolution to: {cmd_arg1}x{cmd_arg2}")

        # Now move the mouse to 1000,1000
        command = 5 # Move mouse
        cmd_arg1 = 1000      # x
        cmd_arg2 = 1000      # y
        
        ser.write( startmarker.to_bytes(1, 'big') )
        ser.write( command.to_bytes(1, 'big') )
        ser.write( cmd_arg1.to_bytes(2, 'big') )
        ser.write( cmd_arg2.to_bytes(2, 'big') )
        ser.write( endmarker.to_bytes(1, 'big') )

        # Now send a right click
        command = 6 # Move mouse
        cmd_arg1 = 3         # right click
        
        ser.write( startmarker.to_bytes(1, 'big') )
        ser.write( command.to_bytes(1, 'big') )
        ser.write( cmd_arg1.to_bytes(1, 'big') )
        ser.write( endmarker.to_bytes(1, 'big') )

        # Now set up, so it keeps pressing key 49 (= the 1 key), with a 50ms delay
        command = 7
        cmd_arg1 = 49
        cmd_arg2 = 50

        ser.write( startmarker.to_bytes(1, 'big') )
        ser.write( command.to_bytes(1, 'big') )
        ser.write( cmd_arg1.to_bytes(1, 'big') )
        ser.write( cmd_arg2.to_bytes(1, 'big') )
        ser.write( endmarker.to_bytes(1, 'big') )
        

except Exception as e:
  print(e)
  print("An exception occurred")
finally:
    ser.close()
    print("Serial connection closed")
