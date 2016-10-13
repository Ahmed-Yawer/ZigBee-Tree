# ZigBee-Tree

This program is implementing the tree ruoting.

First: addressing and joinning the network:-
---------------------------------------------
the node with the default address (0.1) is considered as the corrdinator and been give the addess (0.0)
this coordinator also set the Cm, Rm and Lm of the tree and calculate the addresses range of this netwotk.

other node will send a broadcast message (JOIN_REQ) each random priode 1 to 2 sec to join the network
when the coordinator (or any node that been join the network) recive the broadcast through the broadcast rec method, it reply by sening the uni-cast message as (JOIN_CONF) with the network parametat and the address range assigned to that node.
then that node will recive that uni-cast message throught the uni-cast rec method and then set the first address of the assigned range as it's owne address and make the rest avaialbe to assign to other node to become it's children.
when any node recive a JOIN_CONF it will stop sending the broadcast message. and it will be open to listen to other nodes broadcast so it can make them it's children.
when any node give addresses to three nodes it will stop giving any more addresses.

second: sending packet and forwording:-
---------------------------------------
when press the botton on any node it will send a message (hello) to random node.
in the forward method of the multi hope connection, each node will check wither the dest address is in the one of it's children or not, it will froward the message to that child otherwise it will forward it to it's parent.
the recv method of the multi hope connection will run when the (hello) message reach it's deat and it will print the sender address and it's owne address and the message.
