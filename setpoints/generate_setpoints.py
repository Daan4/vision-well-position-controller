def generate_setpoints(filename, initial_offset_y, initial_offset_x, offset_x, offset_y, rows, columns):
    """Write setpoints indicating the centers of wells in a well plate to a csv file.
       Each line can be interpreted as a tuple (setpoint_x, setpoint_x), with setpoints given in the same unit as the parameters.

       Well order will be (for example for a 24 well plate): A1...A6, B6...B1, C1..C6 etc.

    Args:
        filename: filename/path to the file to be generated
        initial_offset_x: initial offset from the corner of the plate to well center a1
        initial_offset_y: initial offset from the corner of the plate to well center a1
        offset_x: distance between wells in x direction
        offset_y: distance between wells in y direction
        rows: number of wells in y direction
        columns: number of wells in x direction
    """

    with open(filename, 'w') as f:
        for y in range(rows):
            lines_buffer = []
            for x in range(columns):
                # Invert setpoints to work with current hardware setup
                setpoint_x = -round(initial_offset_x + x * offset_x, 2)
                setpoint_y = -round(initial_offset_y + y * offset_y, 2)
                lines_buffer.append("{}, {}\n".format(setpoint_x, setpoint_y))
            if y % 2 != 0:
                lines_buffer.reverse()
            for line in lines_buffer:
                f.write(line)


if __name__ == '__main__':
    # settings for 24 well plate, assuming we start at the topleft corner of the plate
    # generate_setpoints("setpoints\\24.csv", 13.49, 15.13, 19.5, 19.5, 4, 6)

    # settings for 24 well plate, assuming that we start on well A1
    #generate_setpoints("24.csv", 0, 0, 19.5, 19.5, 4, 6)

    # settings for 48 well plate, assuming that we start on well A1
    generate_setpoints("48.csv", 0, 0, 13, 13, 6, 8)
