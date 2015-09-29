#!/usr/bin/perl -w


use IO::Socket::INET;
# auto-flush on socket
$| = 1;
# creating a listening socket

my $socket = new IO::Socket::INET (
 LocalHost => '127.0.0.1',
 LocalPort => '7777',
 Proto => 'udp'
);
die "cannot create socket $!\n" unless $socket;


print "server waiting for client connection on port 7777\n";

my $MAXLEN=4096;

my $msg;

while ($socket->recv($msg, $MAXLEN)) {
    my($port, $ipaddr) = sockaddr_in($socket->peername);
    my $hishost = gethostbyaddr($ipaddr, AF_INET);
    print "Client $hishost said $msg\n";
} 
die "recv: $!";
$socket->close();
exit;

