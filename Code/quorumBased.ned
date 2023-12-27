simple Node{	
	gates:
		input in[];
		output out[];	
}

simple Coordinator{
    parameters:
        int n;
        int nw;
        int nr;
	gates:
		input in[];
		output out[];
		input inFromC;
		output outToC;
		
}

simple Client{
	gates:
		input in;
		output out;

}

network net_QuorumBased{

	parameters:
		int N; 
		int NW;
		int NR;
		int NumClient;
	types:
        channel Channel1 extends ned.DelayChannel {
            delay = 100ms;
        }
        channel Channel2 extends ned.DelayChannel {
            delay = 200ms;
        }
         channel Channel3 extends ned.DelayChannel {
            delay = 500ms;
        }
         channel Channel4 extends ned.DelayChannel {
            delay = 800ms;
        }
         channel Channel5 extends ned.DelayChannel {
            delay = 1000ms;
        }
         channel Channel6 extends ned.DelayChannel {
            delay = 1300ms;
        }
         channel Channel7 extends ned.DelayChannel {
            delay = 700ms;
        }
        
	submodules:
	    client[NumClient]: Client;
		node[N]: Node;
		coord[NumClient]: Coordinator{
		    parameters: 
		    	n = parent.N;
		    	nw = parent.NW;
		    	nr = parent.NR;}
		
		
	connections:
	    
	    for i=0..N-1, for k=0..NumClient-1, if (i+k)%7==0{
	        coord[k].out++ --> Channel1 --> node[i].in++;
	        coord[k].in++ <-- Channel1 <-- node[i].out++;
	 	}
	 	for i=0..N-1, for k=0..NumClient-1, if (i+k)%7==1{
	        coord[k].out++ --> Channel2 --> node[i].in++;
	        coord[k].in++ <-- Channel2 <-- node[i].out++;
	 	}
	 	for i=0..N-1, for k=0..NumClient-1, if (i+k)%7==2{
	        coord[k].out++ --> Channel3 --> node[i].in++;
	        coord[k].in++ <-- Channel3 <-- node[i].out++;
	 	}
	 	for i=0..N-1, for k=0..NumClient-1, if (i+k)%7==3{
	        coord[k].out++ --> Channel4 --> node[i].in++;
	        coord[k].in++ <-- Channel4 <-- node[i].out++;
	 	}
	 	for i=0..N-1, for k=0..NumClient-1, if (i+k)%7==4{
	        coord[k].out++ --> Channel5 --> node[i].in++;
	        coord[k].in++ <-- Channel5 <-- node[i].out++;
	 	}
	 	for i=0..N-1, for k=0..NumClient-1, if (i+k)%7==5{
	        coord[k].out++ --> Channel6 --> node[i].in++;
	        coord[k].in++ <-- Channel6 <-- node[i].out++;
	 	}
	 	for i=0..N-1, for k=0..NumClient-1, if (i+k)%7==6{
	        coord[k].out++ --> Channel7 --> node[i].in++;
	        coord[k].in++ <-- Channel7 <-- node[i].out++;
	 	}
	 	
	    
	    for j=0..NumClient-1{
	        coord[j].outToC --> Channel1 --> client[j].in;
			coord[j].inFromC <-- Channel2 <-- client[j].out;
	    }
}