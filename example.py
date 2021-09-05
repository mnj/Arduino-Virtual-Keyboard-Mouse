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
        sub_cmd1 = 2560     # Width
        sub_cmd2 = 1440     # Height
        
        ser.write( startmarker.to_bytes(1, 'big') )
        ser.write( command.to_bytes(1, 'big') )
        ser.write( sub_cmd1.to_bytes(2, 'big') )
        ser.write( sub_cmd2.to_bytes(2, 'big') )
        ser.write( endmarker.to_bytes(1, 'big') )

        print(f"Set mouse resolution to: {sub_cmd1}x{sub_cmd2}")

        # Now move the mouse to 1000,1000
        command = 5 # Move mouse
        sub_cmd1 = 1000      # x
        sub_cmd2 = 1000      # y
        
        ser.write( startmarker.to_bytes(1, 'big') )
        ser.write( command.to_bytes(1, 'big') )
        ser.write( sub_cmd1.to_bytes(2, 'big') )
        ser.write( sub_cmd2.to_bytes(2, 'big') )
        ser.write( endmarker.to_bytes(1, 'big') )

        # Now send a right click
        command = 6 # Move mouse
        sub_cmd1 = 3         # right click
        
        ser.write( startmarker.to_bytes(1, 'big') )
        ser.write( command.to_bytes(1, 'big') )
        ser.write( sub_cmd1.to_bytes(1, 'big') )
        ser.write( endmarker.to_bytes(1, 'big') )

        # Now set up, so it keeps pressing key 49 (= the 1 key), with a 50ms delay
        command = 7
        sub_cmd1 = 49
        sub_cmd2 = 50

        ser.write( startmarker.to_bytes(1, 'big') )
        ser.write( command.to_bytes(1, 'big') )
        ser.write( sub_cmd1.to_bytes(1, 'big') )
        ser.write( sub_cmd2.to_bytes(1, 'big') )
        ser.write( endmarker.to_bytes(1, 'big') )
        

except Exception as e:
  print(e)
  print("An exception occurred")
finally:
    ser.close()
    print("Serial connection closed")
