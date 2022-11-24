import random
from itertools import count
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import csv

plt.style.use('fivethirtyeight')

x_values = []
y_values = []

index = count()

def animate(i):

    with open('plot_data.csv') as file:
        i = 0
        new_list = []
        for line in file:
            if i == 0:
                continue

        # lines = [re.sub(r'","', r',', line) for line in file]

    data = pd.read_csv(StringIO('\n'.join(lines)))

    # data = pd.read_csv('plot_data.csv')\
    x_values = data['Time']
    y_values = data['Percentage']
    print('x:', x_values)
    print("=-=====")
    print('y:', y_values)

# animate(1)
with open('sender_info.csv') as file:
    csv_reader = csv.reader(file, delimiter=',')
    i = 0
    new_list = []
    for line in csv_reader:
        if i == 0:
            print(line)
            i += 1
        else:
            print(line)
            i += 1

    print(f'processed {i} lines')
        # print(line)
        # if i == 0:
        #     continue




#     plt.cla()
#     plt.plot(x_values, y_values)
#     plt.xlabel('Time')
#     plt.ylabel('Percentage')
#     plt.title('Drop Rate')
#     plt.gcf().autofmt_xdate()
#     plt.tight_layout()
#
# ani = FuncAnimation(plt.gcf(), animate, 5000)
#
# plt.tight_layout()
# plt.show()
