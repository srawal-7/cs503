1. How does the remote client determine when a command's output is fully received from the server, and what techniques can be used to handle partial reads or ensure complete message transmission?

In a remote client-server shell, the client determines the end of a command's output by receiving an End-of-File (EOF) signal. To handle partial reads and ensure complete message transmission, clients can utilize techniques like buffering, where the received data is accumulated until the EOF or a delimiter is detected. Client can also use a fixed-size message format or a message length header to know how much data to expect.

2. This week's lecture on TCP explains that it is a reliable stream protocol rather than a message-oriented one. Since TCP does not preserve message boundaries, how should a networked shell protocol define and detect the beginning and end of a command sent over a TCP connection? What challenges arise if this is not handled correctly?

A networked shell should define command boundaries using delimiters (ex. newlines). The server then buffers incoming data, detects the defined boundaries, and processes complete commands. If it is not handled correctly, commands might get split or merged, leading to errors, security vulnerabilities, and unpredictable behavior, as the system will struggle to identify where one command ends and another begins.

3. Describe the general differences between stateful and stateless protocols.

Stateful protocols maintain information about past interactions between client and server, requiring each to remember previous exchanges, which can enhance efficiency in complex interactions but increase server load. Stateless protocols, treat each request as an independent transaction, with no memory of prior interactions, simplifying server design and improving scalability but potentially requiring redundant data in each request.

4. Our lecture this week stated that UDP is "unreliable". If that is the case, why would we ever use it?

UDP used in applications where speed and low overhead are prioritized over high reliability, such as streaming and gaming. This also suits applications that handle reliability themselves or those needing broadcast/multicast capabilities, where TCP's connection overhead and error checking would be harmful. Another example would be  DNS Lookups where the speed of retrieving an IP address is more important. 

5. What interface/abstraction is provided by the operating system to enable applications to use network communications?

Operating systems provide network stacks, protocols, and services that allow applications to send and receive data over a network. One of the main anbtraction is sockets API and is used as the primary interface for network communication. This allows applications to create endpoints for network connections, send and receive data over those connections, and manage the underlying network protocols (like TCP and UDP) without needing to directly interact with network hardware. 