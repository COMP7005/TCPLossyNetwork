# TCPLossyNetwork

# Proxy 


# Receiver

  gcc receiver.c -o receiver
  
  ./receiver -p 5000
  


# Sender

  gcc sender.c -o sender
  
    -r receiver_ip
    -x proxy_ip
  
  ./sender -r 127.0.0.1 -p 5000 senderfile.txt

# Python 
  Required packages:
  
    sudo apt install python3-pip
    pip install pandas
    pip install matplotlib
    
  Commands:
  
    python3 live_graph.py -s {PATH}/sender_info.csv
    python3 live_graph.py -p {PATH}/proxy_info.csv
    python3 live_graph.py -r {PATH}/receiver_info.csv
    
