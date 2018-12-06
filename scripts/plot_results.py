'''
Run from project root with: python scripts/plot_results.py
'''
import csv
import re
import os
from os import path
import numpy as np
import matplotlib.pyplot as plt

folders = ['demand', 'demand_sort', 'pre', 'pre_sort']
values = [64, 128, 256, 512]
mult_pts = {} # dictionary: (paging_type, value) : (mean, standard_deviation)
map_pts = {}

def read_results():
    '''
    Read each csv file from folders in results/
    '''
    pattern = re.compile(r'\d+')
    results_path = './results'
    for folder in folders:
        folder_path = '{}/{}'.format(results_path, folder)
        files = [f for f in os.listdir(results_path + '/' + folder) if path.isfile(path.join(folder_path, f))]
        for f in files:
            file_path = path.join(folder_path, f)
            mult = []
            mp = []
            value = int(re.findall(pattern, file_path)[0])
            with open(file_path, 'r') as fp:
                reader = csv.reader(fp, delimiter=',')
                next(reader, None)
                for line in reader:
                    # print(line)
                    mp.append(int(line[0].strip()))
                    mult.append(int(line[1].strip()))
                ave_mp = np.mean(mp)
                std_mp = np.std(mp)
                ave_mult = float(np.mean(mult))
                std_mult = float(np.std(mult))
                map_pts[(folder, value)] = (ave_mp, std_mp)
                mult_pts[(folder, value)] = (ave_mult, std_mult)

def plot_pts():
    graph_path = './graphs'
    for folder in folders:
        mult_times = ([], [])
        map_times = ([], [])
        total_times = ([], [])
        for value in values:
            ave_mp, std_mp = map_pts[folder, value]
            ave_mult, std_mult = mult_pts[folder, value]
            std_tot = np.sqrt(std_mp ** 2 + std_mult ** 2)

            map_times[0].append(ave_mp)
            map_times[1].append(std_mp)
            mult_times[0].append(ave_mult)
            mult_times[1].append(std_mult)
            total_times[0].append(ave_mp + ave_mult)
            total_times[1].append(std_tot)
        if 'sort' in folder:
            plt.figure()
            plt.xlabel('Size')
            plt.ylabel('Time (microseconds)')
            title = 'Quicksort {} paging allocation runtimes'.format(folder.replace('_sort', ''))
            plt.title(title)
            plt.errorbar(values, map_times[0], yerr=map_times[1], fmt='o-')
            plt.draw()
            plt.savefig('{}/{}/alloc.png'.format(graph_path, folder))
            plt.close()

            plt.figure()
            plt.xlabel('Size')
            plt.ylabel('Time (log(microseconds))')
            title = 'Quicksort {} paging sorting runtimes'.format(folder.replace('_sort', ''))
            plt.title(title)
            plt.errorbar(values, mult_times[0], yerr=mult_times[1], fmt='o-')
            plt.draw()
            plt.savefig('{}/{}/sort.png'.format(graph_path, folder))
            plt.close()

            plt.figure()
            plt.xlabel('Size')
            plt.ylabel('Time (log(microseconds))')
            title = 'Quicksort {} paging total runtimes'.format(folder.replace('_sort', ''))
            plt.title(title)
            plt.errorbar(values, total_times[0], yerr=total_times[1], fmt='o-')
            plt.draw()
            plt.savefig('{}/{}/total.png'.format(graph_path, folder))
        else:
            plt.figure()
            plt.xlabel('Size')
            plt.ylabel('Time (microseconds)')
            title = 'Matrix Multiplication {} paging allocation runtimes'.format(folder)
            plt.title(title)
            plt.errorbar(values, map_times[0], yerr=map_times[1], fmt='o-')
            plt.draw()
            plt.savefig('{}/{}/alloc.png'.format(graph_path, folder))
            plt.close()

            plt.figure()
            plt.xlabel('Size')
            plt.ylabel('Time (log(microseconds))')
            plt.yscale('log')
            title = 'Matrix Multiplication {} paging multiplication runtimes'.format(folder)
            plt.title(title)
            plt.errorbar(values, mult_times[0], yerr=mult_times[1], fmt='o-')
            plt.draw()
            plt.savefig('{}/{}/mult.png'.format(graph_path, folder))
            plt.close()

            plt.figure()
            plt.xlabel('Size')
            plt.ylabel('Time (log(microseconds))')
            plt.yscale('log')
            title = 'Matrix Multiplication {} paging total runtimes'.format(folder)
            plt.title(title)
            plt.errorbar(values, total_times[0], yerr=total_times[1], fmt='o-')
            plt.draw()
            plt.savefig('{}/{}/total.png'.format(graph_path, folder))

if __name__ == '__main__':
    read_results()
    plot_pts()