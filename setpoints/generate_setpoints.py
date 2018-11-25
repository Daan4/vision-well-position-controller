def generate_setpoints(filename, initial_offset_y, initial_offset_x, offset_x, offset_y, rows, columns):
    """Write setpoints to testsetpoints.csv
    Args:
        filename: name of generated csv file
        initial_offset_x: initial offset from corner to well center a1 -- for now this is hardcoded in globals.py, leave at 0
        initial_offset_y: initial offset from corner to well center a1 -- for now this is hardcoded in globals.py, leave at 0
        offset_x: distance between wells in x direction
        offset_y: distance between wells in y direction
        rows: number of wells in y direction
        columns: number of wells in x direction
    """

    with open(filename, 'w') as f:
        for y in range(rows):
            lines_buffer = []
            for x in range(columns):
                # Compensate hysteresis on x axis by changing the setpoints
                if y % 2 != 0 and x != columns - 1:
                    setpoint_x = round(initial_offset_x + x * offset_x, 2)
                elif y % 2 == 0 and (x == 0 and y != 0):
                    setpoint_x = round(initial_offset_x + x * offset_x, 2)
                else:
                    setpoint_x = round(initial_offset_x + x * offset_x, 2)
                setpoint_y = round(initial_offset_y + y * offset_y, 2)
                lines_buffer.append("{}, {}\n".format(setpoint_x, setpoint_y))
            if y % 2 != 0:
                lines_buffer.reverse()
            for line in lines_buffer:
                f.write(line)


if __name__ == '__main__':
    # settings for 24 well plate
    generate_setpoints("setpoints\\24.csv", 13.49, 15.13, 19.5, 19.5, 4, 6)