## Title O3D Create Test Data
#
#   Purpose: This example Python program simulates data received from a
#   sensor by writing the expected format of example data to a file. The
#   visualization will be demonstrated using a Python module called Open3D.
#
#   Special notes:
#       1. Open3D only works with Pythons 3.6-3.9.  It does not work with 3.10
#       2. For this eample you should run it in IDLE.  Anaconda/Conda/Jupyter
#       require different Open3D graphing methods (these methods are poorly documented)
#       3. Under Windows 10 you may need to install the MS Visual C++ Redistributable bundle
#           https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170
#       4. VirtualBox does not support OpenGL. If you're running Windows under Virtualbox or
#       your system doesn't support OpenGL (very rare), then you can install an OpenGL emulator dll
#           https://fdossena.com/?p=mesa/index.frag (unzip and copy opengl32.dll into Python dir)
#


import sys

sys.path.append(r'C:/python39/lib/site-packages')
import serial
import numpy as np
import open3d as o3d

import time





if __name__ == "__main__":
    s = serial.Serial('COM5',baudrate=115200,timeout=10)
    #print("Opening: " + s.name)
    steps = input("how many steps will you be taking?")
    f = open("graph.xyz", "w")    #create a new file for writing 

    # reset the buffers of the UART port to delete the remaining data in the buffers
    s.reset_output_buffer()
    s.reset_input_buffer()

    # wait for user's signal to start the program
    #input("Press Enter to start communication...")
    # send the character 's' to MCU via UART
    # This will signal MCU to start the transmission
    # wait for user's signal to start the program
    #input("Press Enter to start communication...")
# send the character 's' to MCU via UART
# This will signal MCU to start the transmission
    s.write('s'.encode())

    for q in range(int(steps)):

        for l in range(0,(30)):
            x = s.readline()

            coor=x.decode()
            f.write(coor)
            print(coor)
       
           
    # the encode() and decode() function are needed to convert string to bytes
    # because pyserial library functions work with type "bytes"

    f.close()
    #close the port
    #print("Closing: " + s.name)
    s.close()
    
    
    #Read the test data in from the file we created        
    print("Read in the prism point cloud data (pcd)")
    pcd = o3d.io.read_point_cloud("graph.xyz", format="xyz")

    #Lets see what our point cloud data looks like numerically       
    print("The PCD array:")
    print(np.asarray(pcd.points))

    #Lets see what our point cloud data looks like graphically       
    print("Lets visualize the PCD: (spawns seperate interactive window)")
    o3d.visualization.draw_geometries([pcd])

    #OK, good, but not great, lets add some lines to connect the vertices
    #   For creating a lineset we will need to tell the packahe which vertices need connected
    #   Remember each vertex actually contains one x,y,z coordinate

    #Give each vertex a unique number
    
    yz_slice_vertex = []
    for x in range(0,(30*int(steps))):#120
        yz_slice_vertex.append([x])

    #Define coordinates to connect lines in each yz slice        
    lines = []  
    for x in range(0,(30*int(steps)),30): #0,120,30
        for k in range(0,30):

            if (k == 29):
                lines.append([yz_slice_vertex[x+k], yz_slice_vertex[x]])
            else:
                lines.append([yz_slice_vertex[x+k], yz_slice_vertex[x+k+1]])

    #Define coordinates to connect lines between current and next yz slice        
    for x in range(0,((30*int(steps))-31),30):#89
        for k in range(0,30):
            lines.append([yz_slice_vertex[x+k], yz_slice_vertex[x+k+30]])
    #This line maps the lines to the 3d coordinate vertices
    line_set = o3d.geometry.LineSet(points=o3d.utility.Vector3dVector(np.asarray(pcd.points)),lines=o3d.utility.Vector2iVector(lines))

    #Lets see what our point cloud data with lines looks like graphically       
    o3d.visualization.draw_geometries([line_set])

