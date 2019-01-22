"""The WPCLogParser class in this module is intended for use with the .csv log files generated by the
WellPositionController class in well_position_controller.py .
Its functions can plot and calculate interesting data from the logs."""
import csv
import re
from enum import Enum
from datetime import datetime
from ast import literal_eval
import numpy as np
from math import sqrt
import matplotlib.pyplot as plt
import shutil
import os
import imageio

TIMESTAMPFORMAT = '%Y%m%d%H%M%S'


class Headers(Enum):
    """Known log headers, only the first word (after splitting the header on spaces) has to be given."""
    TIMESTAMP = 'Timestamp'
    TARGET = 'Target'
    SETPOINT = 'Setpoint'
    TOTAL = 'Total'
    TOTALpx = 'Totalpx'
    TOTALmm = 'Totalmm'
    RESULT = 'Pass'


class WPCLogParser():
    def __init__(self, logfile):
        # Load log contents
        with open(logfile, 'r') as f:
            r = csv.reader(f, delimiter=',')
            self.log_rows = [row for row in r]

        # Store header-row index mappings for common headers and evaluator headers
        self.parse_headers()

        # Cast row data from string to their intended type
        self.parse_rows()

    def parse_headers(self):
        # Map columns to known headers or evaluator outputs
        # Assumes the following order:
        # timestamp,
        # target,
        # setpoint,
        # evaluator offset x px,
        # evaluator offset y px,
        # evaluator offset x mm,
        # evaluator offset y mm, (previous 4 columns repeated for each evaluator used)
        # total weighted offset px
        # total weighted offset mm
        # result (pass/fail)
        self.std_col_map = {Headers.TIMESTAMP: 0,
                            Headers.TARGET: 1,
                            Headers.SETPOINT: 2}
        self.eval_col_map = {}
        self.eval_names = []
        for i in range(3, len(self.log_rows[0]), 4):
            first_header_word = self.log_rows[0][i].split(" ")[0]
            if first_header_word == Headers.TOTAL.value:
                # end of evaluator columns
                self.std_col_map[Headers.TOTALpx] = i
                self.std_col_map[Headers.TOTALmm] = i+1
                self.std_col_map[Headers.RESULT] = i+2
                break
            else:
                # Extract weight from header
                weight = float(re.findall(r'\((.*?)\)', self.log_rows[0][i])[0].split(" ")[1])
                # Store evaluator name and evaluator columns
                # eval_col_map order: xpx, ypx, xmm, ymm, weight
                self.eval_names.append(first_header_word)
                self.eval_col_map[self.eval_names[len(self.eval_names)-1]] = [
                    i,
                    i+1,
                    i+2,
                    i+3,
                    weight
                ]
        # Extract error margin from results header
        self.error_margin = re.findall(r'\((.*?)\)', self.log_rows[0][self.std_col_map[Headers.RESULT]])[1]
        self.error_margin = literal_eval(self.error_margin)

        # Remove headers from row list
        self.log_rows.pop(0)

    def parse_rows(self):
        """Cast each value from str to its proper datatype"""
        for row in self.log_rows:
            row[self.std_col_map[Headers.TIMESTAMP]] = datetime.strptime(row[self.std_col_map[Headers.TIMESTAMP]], TIMESTAMPFORMAT)
            row[self.std_col_map[Headers.TARGET]] = literal_eval(row[self.std_col_map[Headers.TARGET]])
            row[self.std_col_map[Headers.SETPOINT]] = literal_eval(row[self.std_col_map[Headers.SETPOINT]])
            for eval_cols in self.eval_col_map.values():
                row[eval_cols[0]] = int(row[eval_cols[0]])
                row[eval_cols[1]] = int(row[eval_cols[1]])
                row[eval_cols[2]] = float(row[eval_cols[2]])
                row[eval_cols[3]] = float(row[eval_cols[3]])
            row[self.std_col_map[Headers.TOTALpx]] = tuple(np.fromstring(row[self.std_col_map[Headers.TOTALpx]][1:-1].replace(".", ""), int, 2, " "))
            row[self.std_col_map[Headers.TOTALmm]] = literal_eval(row[self.std_col_map[Headers.TOTALmm]])
            if row[self.std_col_map[Headers.RESULT]] == '1':
                row[self.std_col_map[Headers.RESULT]] = True
            else:
                row[self.std_col_map[Headers.RESULT]] = False

    def average_required_iterations(self):
        """Calculate the average number of iterations required to position the well from the original position
           to the setpoint"""
        iter_counts = []
        current_iter_count = 0
        for row in self.log_rows:
            current_iter_count += 1
            if row[self.std_col_map[Headers.RESULT]]:
                iter_counts.append(current_iter_count)
                current_iter_count = 0
        return np.average(iter_counts)

    def average_total_error_per_iteration(self):
        """Calculate the average total error per iteration in mm. This is the total distance the load travelled
        before arriving at its target position.
        Total error distance is calculated as if euclidian, even though in reality we only move in straight lines in x and y direction"""
        total_errors = []
        current_total_error = 0
        for row in self.log_rows:
            error_x = row[self.std_col_map[Headers.TOTALmm]][0]
            error_y = row[self.std_col_map[Headers.TOTALmm]][1]
            current_total_error += sqrt(error_x ** 2 + error_y ** 2)
            if row[self.std_col_map[Headers.RESULT]]:
                total_errors.append(current_total_error)
                current_total_error = 0
        return np.average(total_errors)

    def average_time_per_setpoint(self):
        """Calculate the average time it took to reach each setpoint. All times are calculated in seconds"""
        all_setpoint_times = []
        current_setpoint_time = 0
        for i, row in enumerate(self.log_rows[:-1]):  # ignore the last entry, since we can't know how long that one took (there is no next row to check the timestamp of)
            current_timestamp = row[self.std_col_map[Headers.TIMESTAMP]]
            next_timestamp = self.log_rows[i+1][self.std_col_map[Headers.TIMESTAMP]]
            current_setpoint_time += (next_timestamp - current_timestamp).total_seconds()
            if row[self.std_col_map[Headers.RESULT]]:
                all_setpoint_times.append(current_setpoint_time)
                current_setpoint_time = 0
        return np.average(all_setpoint_times)

    def plot_errors(self, setpoint_indices=None, evaluator_names=None, _filename=None, mm=False, colors=False):
        """Plot errors for each setpoint index passed in the parameters setpoint_indices.
        Leave setpoint_indices as None to plot error for each setpoint.
        The plots will show a dot for each location visited around a well.
        Displaying the setpoint index and iteration number.
        The plot represents an image with the target in the center and (x, y) representing the pixel location in the image.
        Each setpoint is color coded

        setpoint_indices: process only setpoints for setpoint INDICES (eg setpoint 0 is generally well A1) given in this list, or all setpoints if None
        evaluator_names: process only evaluator output for evaluator NAMES given in this list, or all evaluators if None
        filename: plot will be saved as filename if it's given
        mm: set to true to plot errors in mm rather than pixels"""
        x = []
        y = []
        setpoint_index = 0
        if setpoint_indices is None:
            setpoint_indices = range(len(self.log_rows))
        if evaluator_names is None:
            evaluator_names = self.eval_names
        fig, ax = plt.subplots()
        ax.set_title("Visualization of positioning error")
        if mm:
            ax.set(xlabel="x error [mm]", ylabel="y error [mm]")
        else:
            ax.set(xlabel="x error [px]", ylabel="y error [px]")

        for row in self.log_rows:
            curr_x = 0
            curr_y = 0
            if setpoint_index in setpoint_indices:
                for eval_name in evaluator_names:
                    if mm:
                        eval_error_x = row[self.eval_col_map[eval_name][2]]
                        eval_error_y = row[self.eval_col_map[eval_name][3]]
                    else:
                        eval_error_x = row[self.eval_col_map[eval_name][0]]
                        eval_error_y = row[self.eval_col_map[eval_name][1]]
                    weight = self.eval_col_map[eval_name][4]
                    curr_x += eval_error_x * weight
                    curr_y += eval_error_y * weight
                x.append(curr_x)
                y.append(curr_y)
            if row[self.std_col_map[Headers.RESULT]]:
                setpoint_index += 1
                if colors:
                    ax.scatter(x, y, s=75, edgecolor='none', alpha=0.5, label=str(setpoint_index))
                    x = []
                    y = []
        # plot results
        if not colors:
            ax.scatter(x, y, s=75, edgecolor='none', alpha=0.5, label=str(setpoint_index))
        ax.grid(True)

        if _filename is not None:
            fig.savefig(_filename)

        plt.show()
        
    def rename_correct_images(self, foldername=None):
        """Make a copy of correct images in /images/ named by well 
        by comparing the image timestamp to the log timestamp"""
        well_counter = 1

        if foldername:
            if not os.path.isdir('images/{}'.format(foldername)):
                os.mkdir('images/{}'.format(foldername))

        for row in self.log_rows:
            if row[self.std_col_map[Headers.RESULT]]:
                timestampstring = row[self.std_col_map[Headers.TIMESTAMP]].strftime(TIMESTAMPFORMAT)
                if foldername is None:
                    foldername = timestampstring
                    if not os.path.isdir('images/{}'.format(foldername)):
                        os.mkdir('images/{}'.format(foldername))
                try:
                    input_filename = 'images/{}.png'.format(timestampstring)
                    output_filename = 'images/{}/{}.png'.format(foldername, well_counter)
                    shutil.copy(input_filename, output_filename)
                except IOError as _:
                    print("file for well index {} not found: {}".format(well_counter, input_filename))
                well_counter += 1


def generate_gif(image_dir, output_filename, fps=0.2):
    """Used to convert the images outputted by rename_correct_image to a gif"""
    images = []
    for filename in os.listdir(image_dir):
        if filename.endswith('.png'):
            filepath = os.path.join(image_dir, filename)
            images.append(imageio.imread(filepath))
    imageio.mimsave(output_filename, images)


if __name__ == '__main__':
    #filename = 'logs/50x random error from (start E4 end B4) max error [0.5, 2.5] error margin [0.2, 0.2].csv'
    filename = 'logs/48x well plate with offsets.csv'
    #filename = 'logs/48x well plate without offsets.csv'
    wlp = WPCLogParser(filename)
    print("average required iterations: {:.3f}".format(wlp.average_required_iterations()))
    print("average total error per iteration: {:.3f} mm".format(wlp.average_total_error_per_iteration()))
    print("average time taken per setpoint: {:.3f} s".format(wlp.average_time_per_setpoint()))
    #wlp.rename_correct_images('48x w o offsets')
    wlp.plot_errors(mm=True, _filename='out2.png')
    #generate_gif('images/48x w offsets/', 'images/48x w o offsets/48x w offsets.gif')
