import csv
import sys
import getopt
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

plt.style.use('fivethirtyeight')

x_time = []
y_sent = []
y_received = []
y_sent_drop = []
y_received_drop = []

def animate(i, mode, file):
    try:
        data = pd.read_csv(file)
        
        if mode in ["Sender", "Receiver"]:
            x_time = data["Time"]
            y_sent = data["Sent"]
            y_received = data["Received"]

            plt.cla()
            plt.plot(x_time, y_sent, label = "Sent", alpha=0.8, linestyle="dotted", linewidth=8)
            plt.plot(x_time, y_received, label = "Received", alpha=0.8, linestyle="dashed", linewidth=5)

        elif mode in ["Proxy"]:
            x_time = data["Time"]
            y_sent = data["Sent"]
            y_received = data["Received"]
            y_sent_drop = data["Sent_Drop"]
            y_received_drop = data["Received_Drop"]

            plt.cla()
            plt.plot(x_time, y_sent, label = "Sent", alpha=0.8, linestyle="dotted", linewidth=8)
            plt.plot(x_time, y_received, label = "Received", alpha=0.8, linestyle="dashed", linewidth=5)
            plt.plot(x_time, y_sent_drop, label = "Sent_Drop", alpha=0.8, linestyle=(0, (1, 1)), linewidth=8)
            plt.plot(x_time, y_received_drop, label = "Received_Drop", alpha=0.8, linestyle=(0, (3, 1, 1, 1)), linewidth=5)

        else:
            print("There is no such mode: ", mode)
            sys.exit(2)

        plt.xlabel("Time")
        plt.ylabel("# of Packets")
        plt.title("{} Stats".format(mode))
        plt.gcf().autofmt_xdate()
        # plt.tight_layout()
        plt.legend()

    except Exception as e:
        #print(e)
        # sys.exit(2)
        return

def main():
    argv = sys.argv
    arg_file = mode= ""
    arg_help = "{0} -s/-p/-r <csv_file>".format(argv[0])

    try:
        opts, args = getopt.getopt(argv[1:], "hs:r:p:", ["help", "input="])
    except:
        print(arg_help)
        sys.exit(2)

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            print(arg_help)
            sys.exit(2)
        elif opt in ("-s", "--csv_file"):
            arg_file = arg
            mode = "Sender"
        elif opt in ("-p", "--csv_file"):
            arg_file = arg
            mode = "Proxy"
        elif opt in ("-r", "--csv_file"):
            arg_file = arg
            mode = "Receiver"

    print("File:", arg_file)
    print("Mode:", mode)

    try:
        animation = FuncAnimation(plt.gcf(), animate, 5000, fargs=(mode, arg_file, ))
        plt.tight_layout()
        plt.show()
    except KeyboardInterrupt as e:
        print("Bye")
        # sys.exit(2)

if __name__ == "__main__":
    main()

