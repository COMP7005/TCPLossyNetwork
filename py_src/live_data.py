
import csv
import time
from datetime import datetime
import random

with open('plot_data.csv', 'w') as f:
    writer = csv.writer(f)
    writer.writerow(['Time', 'Percentage'])

syn = ack = 0
while True:
    syn += 1
    ack += random.randint(0, 100)
    now = datetime.now().strftime("%H:%M:%S")
    print(round(ack/syn*100, 1))
    row = [now, round(ack/syn*100, 1)]

    with open('plot_data.csv', 'a') as f:
        writer = csv.writer(f)
        writer.writerow(row)

    time.sleep(1)
