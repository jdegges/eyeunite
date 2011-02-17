#include "tree.h"

#include <netdb.h>

#define EU_ADDRSTRLEN INET6_ADDRSTRLEN
#define EU_TOKENSTRLEN 224

int main (void){


	char pid1[EU_TOKENSTRLEN] = "1";
	char addr1[EU_ADDRSTRLEN] = "ip1\0";

	char pid2[EU_TOKENSTRLEN] = "2\0";
	char addr2[EU_ADDRSTRLEN] = "ip2\0";

	char pid3[EU_TOKENSTRLEN] = "3\0";
	char addr3[EU_ADDRSTRLEN] = "ip3\0";

	char pid4[EU_TOKENSTRLEN] = "4\0";
	char addr4[EU_ADDRSTRLEN] = "ip4\0";

	char pid5[EU_TOKENSTRLEN] = "5\0";
	char addr5[EU_ADDRSTRLEN] = "ip5";

	char pid6[EU_TOKENSTRLEN] = "6\0";
	char addr6[EU_ADDRSTRLEN] = "ip6\0";

	char pid7[EU_TOKENSTRLEN] = "7\0";
	char addr7[EU_ADDRSTRLEN] = "ip7\0";

	char pid8[EU_TOKENSTRLEN] = "8\0";
	char addr8[EU_ADDRSTRLEN] = "ip8\0";

	char pid9[EU_TOKENSTRLEN] = "9\0";
	char addr9[EU_ADDRSTRLEN] = "ip9\0";

	char pid10[EU_TOKENSTRLEN] = "10\0";
	char addr10[EU_ADDRSTRLEN] = "ip10\0";

	struct tnode* tree = initialize(100, pid1, addr1);

	addPeer(tree, 20, 200, pid2, addr2);
	addPeer(tree, 30, 300, pid3, addr3);
	addPeer(tree, 40, 400, pid4, addr4);
	addPeer(tree, 50, 500, pid5, addr5);
	addPeer(tree, 50, 500, pid6, addr6);
	addPeer(tree, 50, 500, pid7, addr7);
	addPeer(tree, 50, 500, pid8, addr8);
	addPeer(tree, 50, 500, pid9, addr9);
	addPeer(tree, 50, 500, pid10, addr10);

	//printTree(tree);


	movePeer(tree, pid10);

	//printTree(tree);


	removePeer(tree, pid3);

	printTree(tree);

}	
