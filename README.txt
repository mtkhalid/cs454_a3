Names:
Sivakumaran Sivanathan – ssivanat - 
Talha Khalid - mtkhalid -


---Open 3 terminals---
1. Open three terminals

---Make the Client, Binder, Server---
2. In the first terminal, run “make clean”. Then run “make”. (Note: you can also individually build the client, binder, server via "make client" and "make binder" and "make server")

---Running the binder---
3. In the first terminal, type ./binder to run the binder
4. Now you will see the hostname and port number the binder is running on. Please make a note of these, as you will need to use them in the remaining 2 terminals.
5. In Terminal 2 and 3, type "export BINDER_ADDRESS=$HOSTNAME" and "export BINDER_PORT=$PORTNUMBER", whereby $HOSTNAME and $PORTNUMBER correspond to the host name and port number from Step 4. Now, you have accordingly set the environment variables for the binder in these terminals.

---Running the server---
In terminal 2, run the server via "./server"

---Running the client---
In terminal 3, run the client via "./client"


Acknowledgements:
We utilized a lot of sample code available at Beej's Guide: <http://beej.us/guide/bgnet/output/html/multipage/index.html>
