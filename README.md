
# Introduction to Computer Communications Course 
## Tel Aviv University - Spring Semester 2022

### 1) Proramming Assignment 1: Noisy Channel
> **How to run:**   
> a) Run the channel: 
> > channel.exe [flag] [additional_parameters]
> > - Deterministic: [flag] = -d, [additional_parameters] = [n'th-bit]  
> > Flips every n'th bit of the data.
> > - Random: [flag] = -r, [additional_parameters] = [prob] [seed]  
> > Sets the random seed in accordance to the given [seed], flips the data bits randomly with probabilty of $\frac{P}{2^{16}}$ where P is [prob].  
> > The channel prints IP addresses and ports for the sender and receiver. 

> b) Run sender and receiver with the IP addresses and ports given by the channel:
> > sender.exe [sender_IP_address] [sender_port]  
> > receiver.exe [receiver_IP_address] [receiver_port]  
> > Where [sender_IP_address] [sender_port], [receiver_IP_address] [receiver_port] are the IP addresses and ports given to the sender and the receiver repectivly by the channel.    

### 2) Proramming Assignment 1: DNS client
