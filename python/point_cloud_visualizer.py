"""
3D LiDAR Point Cloud Visualizer

This script receives distance measurements from the MSP432 microcontroller via UART,
converts them to Cartesian coordinates, and visualizes the resulting point cloud
using Open3D.

Requirements:
    - Python 3.6-3.9 (Open3D compatibility)
    - pyserial
    - numpy
    - open3d

Note: Open3D only works with Python 3.6-3.9. It does not work with Python 3.10+.
For best results, run this script in IDLE. Anaconda/Conda/Jupyter require different
Open3D graphing methods.

Windows 10 users may need to install the MS Visual C++ Redistributable bundle:
https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170

VirtualBox users: VirtualBox does not support OpenGL. If running Windows under VirtualBox
or your system doesn't support OpenGL, install an OpenGL emulator DLL:
https://fdossena.com/?p=mesa/index.frag (unzip and copy opengl32.dll into Python dir)
"""

import sys
import serial
import numpy as np
import open3d as o3d
import time

# Configuration
SERIAL_PORT = 'COM5'  # Change to your serial port (e.g., '/dev/ttyUSB0' on Linux)
BAUDRATE = 115200
TIMEOUT = 10
MEASUREMENTS_PER_SWEEP = 30  # Number of measurements per 360-degree sweep


def acquire_data(serial_port, num_sweeps):
    """
    Acquire point cloud data from the microcontroller via UART.
    
    Args:
        serial_port: Serial port object
        num_sweeps: Number of 360-degree sweeps to perform
        
    Returns:
        str: Path to the output XYZ file
    """
    output_file = "graph.xyz"
    
    # Open serial connection
    print(f"Opening serial port: {serial_port.name}")
    
    # Reset UART buffers
    serial_port.reset_output_buffer()
    serial_port.reset_input_buffer()
    
    # Send start command to microcontroller
    serial_port.write('s'.encode())
    
    # Open output file for writing
    with open(output_file, "w") as f:
        # Acquire data for each sweep
        for sweep in range(num_sweeps):
            print(f"Acquiring sweep {sweep + 1}/{num_sweeps}...")
            
            # Read measurements for one complete sweep
            for measurement in range(MEASUREMENTS_PER_SWEEP):
                try:
                    # Read one line of XYZ coordinates
                    line = serial_port.readline()
                    coordinates = line.decode('utf-8', errors='ignore')
                    f.write(coordinates)
                    print(coordinates.strip())
                except Exception as e:
                    print(f"Error reading data: {e}")
                    continue
    
    print(f"Data acquisition complete. Saved to {output_file}")
    return output_file


def visualize_point_cloud(xyz_file):
    """
    Load and visualize point cloud data using Open3D.
    
    Args:
        xyz_file: Path to XYZ point cloud file
    """
    # Read point cloud from file
    print(f"Reading point cloud from {xyz_file}...")
    pcd = o3d.io.read_point_cloud(xyz_file, format="xyz")
    
    # Display point cloud data numerically
    print("\nPoint cloud data (first 10 points):")
    points_array = np.asarray(pcd.points)
    print(points_array[:10])
    print(f"Total points: {len(points_array)}")
    
    # Visualize point cloud
    print("\nVisualizing point cloud (opens interactive window)...")
    o3d.visualization.draw_geometries([pcd])
    
    # Create line set to connect points
    print("\nCreating line connections...")
    num_points = len(points_array)
    num_sweeps = num_points // MEASUREMENTS_PER_SWEEP
    
    # Create vertex indices
    vertices = []
    for i in range(num_points):
        vertices.append([i])
    
    # Connect points within each sweep (circular connections)
    lines = []
    for sweep_idx in range(num_sweeps):
        sweep_start = sweep_idx * MEASUREMENTS_PER_SWEEP
        for point_idx in range(MEASUREMENTS_PER_SWEEP - 1):
            lines.append([vertices[sweep_start + point_idx], 
                         vertices[sweep_start + point_idx + 1]])
        # Close the loop
        lines.append([vertices[sweep_start + MEASUREMENTS_PER_SWEEP - 1], 
                     vertices[sweep_start]])
    
    # Connect corresponding points between sweeps
    for sweep_idx in range(num_sweeps - 1):
        sweep_start = sweep_idx * MEASUREMENTS_PER_SWEEP
        next_sweep_start = (sweep_idx + 1) * MEASUREMENTS_PER_SWEEP
        for point_idx in range(MEASUREMENTS_PER_SWEEP):
            lines.append([vertices[sweep_start + point_idx], 
                         vertices[next_sweep_start + point_idx]])
    
    # Create line set
    line_set = o3d.geometry.LineSet(
        points=o3d.utility.Vector3dVector(points_array),
        lines=o3d.utility.Vector2iVector(lines)
    )
    
    # Visualize point cloud with lines
    print("Visualizing point cloud with line connections...")
    o3d.visualization.draw_geometries([line_set])


def main():
    """Main function"""
    # Get number of sweeps from user
    try:
        num_sweeps = int(input("How many sweeps will you be taking? "))
    except ValueError:
        print("Invalid input. Using default: 1 sweep")
        num_sweeps = 1
    
    # Open serial port
    try:
        ser = serial.Serial(SERIAL_PORT, baudrate=BAUDRATE, timeout=TIMEOUT)
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        print(f"Please update SERIAL_PORT in the script to match your system.")
        return
    
    try:
        # Acquire data from microcontroller
        xyz_file = acquire_data(ser, num_sweeps)
        
        # Visualize point cloud
        visualize_point_cloud(xyz_file)
        
    except KeyboardInterrupt:
        print("\nInterrupted by user")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        # Close serial port
        ser.close()
        print(f"Closed serial port: {ser.name}")


if __name__ == "__main__":
    main()

