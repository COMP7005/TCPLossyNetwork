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
