//Purpose: mux of crossbar-based switch that is composed of 7 routers
//Author: Jiayi Sheng
//Organization: CAAD lab @ Boston University
//Start date: Feb 22nd 2015
//
//
////reduction table entry format
/*
* at most five fan-in for 3D-torus network.
for each fanin, format is as below:
|3-bit expect counter|3-bit arrival bookkeeping counter|dst   |table index| weight accumulator| payload accumulator|
|3 bits              | 3 bits                          |4 bits|16 bits    | 8 bits            | 128 bits           | 162 bits in total
* */

module mux
#(
    parameter DataSize=8'd172,
    parameter X=8'd0,
    parameter Y=8'd0,
    parameter Z=8'd0,
    parameter srcID=4'd0,
    parameter ReductionBitPos=254,
    parameter PayloadLen=128,
    parameter DataWidth=256,
    parameter WeightPos=144,
    parameter WeightWidth=8,
    parameter IndexPos=128,
    parameter IndexWidth=16,
    parameter PriorityPos=152,
    parameter PriorityWidth=8,
    parameter ExitPos=160,
    parameter ExitWidth=4,
    parameter InterNodeFIFODepth=128,
    parameter IntraNodeFIFODepth=1,
    parameter RoutingTableWidth=32,
    parameter RoutingTablesize=256,
    parameter MulticastTableWidth=103,
    parameter MulticastTablesize=256,
    parameter ReductionTableWidth=162,
    parameter ReductionTablesize=256,
    parameter PcktTypeLen=4
)
(
    input clk,
    input rst,
    input [DataWidth-1:0] in_local,
    input [DataWidth-1:0] in_yneg,
    input [DataWidth-1:0] in_ypos,
    input [DataWidth-1:0] in_xpos,
    input [DataWidth-1:0] in_xneg,
    input [DataWidth-1:0] in_zpos,
    input [DataWidth-1:0] in_zneg,
    input in_pipeline_stall_local,
    input in_pipeline_stall_yneg,
    input in_pipeline_stall_ypos,
    input in_pipeline_stall_xpos,
    input in_pipeline_stall_xneg,
    input in_pipeline_stall_zpos,
    input in_pipeline_stall_zneg,
    output send,
    output in_avail_local,
    output in_avail_yneg,
    output in_avail_ypos,
    output in_avail_xpos,
    output in_avail_xneg,
    output in_avail_zpos,
    output in_avail_zneg,
    output reg [DataWidth-1:0] out
);
    parameter SrcPacketIDPos=222;
    parameter DstPacketIDPos=186;

    wire [DataWidth-1:0] in[6:0];
    wire in_pipeline_stall[6:0];
    wire in_avail[6:0];

    wire FIFO_empty[6:0];
    wire FIFO_full[6:0];

    reg FIFO_consume[6:0];

    wire [DataWidth-1:0] FIFO_out[6:0];

    wire [PriorityWidth-1:0] priority[6:0];

//    wire [2:0] sel_index;
    reg [PriorityWidth-1:0] priority01; //the higher priority between the 0th port and 1st port
    reg [2:0] sel_index01;
    reg [PriorityWidth-1:0] priority23; //the higher priority between the 2nd and 3rd port
    reg [2:0] sel_index23;
    reg [PriorityWidth-1:0] priority45; //the higher priority between the 4th and 5th port
    reg [2:0] sel_index45;
    reg [PriorityWidth-1:0] priority0123; //the highest priority among the 0th port, 1st port, 2nd port and 3rd port
    reg [2:0] sel_index0123;
    reg [PriorityWidth-1:0] priority456; //the higher priority among 4th port, 5th port and 6th port
    reg [2:0] sel_index456;
    reg [PriorityWidth-1:0] priority0123456; //the highest priority among 0,1,2,3,4,5,6 ports
    reg [2:0] sel_index0123456;

    reg [DataWidth-1:0] sel_data;//the outputs from the buffers that has the highest priority
    reg [DataWidth-1:0] sel_data_RR;

    reg pipeline_stall;

    reg[ReductionTableWidth-1:0] reduction_table[ReductionTablesize-1:0];

    wire is_reduction;
    wire is_reduction_WB;
    wire reduction_ready;

    wire [DataWidth-1:0] reduction_out;

    reg [ReductionTableWidth-1:0] reduction_table_entry;
    wire [ReductionTableWidth-1:0] reduction_table_entry_next;

    wire [2:0] next_counter;


    //this pipeline has severa stages
    //1st stage: pick the highest priority data from seven FIFOs (FR) (fifo read) 
    //2nd stage: read the reduction table (if the data is the reduction data)  (RR) (reduction read)
    //3rd stage: either write back to the reduction table or send to the output (WB)
    

    parameter InterSwitchBufferDepth=4;

    

    assign in_pipeline_stall[0]=in_pipeline_stall_local;
    assign in_pipeline_stall[1]=in_pipeline_stall_yneg;
    assign in_pipeline_stall[2]=in_pipeline_stall_ypos;
    assign in_pipeline_stall[3]=in_pipeline_stall_xpos;
    assign in_pipeline_stall[4]=in_pipeline_stall_xneg;
    assign in_pipeline_stall[5]=in_pipeline_stall_zpos;
    assign in_pipeline_stall[6]=in_pipeline_stall_zneg;

    assign in[0]=in_local;
    assign in[1]=in_yneg;
    assign in[2]=in_ypos;
    assign in[3]=in_xpos;
    assign in[4]=in_xneg;
    assign in[5]=in_zpos;
    assign in[6]=in_zneg;

    assign in_avail_local=in_avail[0];
    assign in_avail_yneg=in_avail[1];
    assign in_avail_ypos=in_avail[2];
    assign in_avail_xpos=in_avail[3];
    assign in_avail_xneg=in_avail[4];
    assign in_avail_zpos=in_avail[5];
    assign in_avail_zneg=in_avail[6];

    assign in_avail[0]=~FIFO_full[0];
    assign in_avail[1]=~FIFO_full[1];
    assign in_avail[2]=~FIFO_full[2];
    assign in_avail[3]=~FIFO_full[3];
    assign in_avail[4]=~FIFO_full[4];
    assign in_avail[5]=~FIFO_full[5];
    assign in_avail[6]=~FIFO_full[6];

//first stage:

    FIFO#(
        .FIFO_depth(InterSwitchBufferDepth),
        .FIFO_width(DataWidth)
    )FIFO_local
    (
        .clk(clk),
        .rst(rst),
        .in(in[0]),
        .produce(~in_pipeline_stall[0] && in[0][DataWidth-1]),
        .consume(FIFO_consume[0]),
        .full(FIFO_full[0]),
        .empty(FIFO_empty[0]),
        .out(FIFO_out[0])
    );

    FIFO#(
        .FIFO_depth(InterSwitchBufferDepth),
        .FIFO_width(DataWidth)
    )
    FIFO_yneg(
        .clk(clk),
        .rst(rst),
        .in(in[1]),
        .produce(~in_pipeline_stall[1] && in[1][DataWidth-1]),
        .consume(FIFO_consume[1]),
        .full(FIFO_full[1]),
        .empty(FIFO_empty[1]),
        .out(FIFO_out[1])
       
    );

    FIFO#(
        .FIFO_depth(InterSwitchBufferDepth),
        .FIFO_width(DataWidth)
    )
    FIFO_ypos(
        .clk(clk),
        .rst(rst),
        .in(in[2]),
        .produce(~in_pipeline_stall[2] && in[2][DataWidth-1]),
        .consume(FIFO_consume[2]),
        .full(FIFO_full[2]),
        .empty(FIFO_empty[2]),
        .out(FIFO_out[2])
       
    );
 
    FIFO#(
        .FIFO_depth(InterSwitchBufferDepth),
        .FIFO_width(DataWidth)
    )
    FIFO_xpos(
        .clk(clk),
        .rst(rst),
        .in(in[3]),
        .produce(~in_pipeline_stall[3] && in[3][DataWidth-1]),
        .consume(FIFO_consume[3]),
        .full(FIFO_full[3]),
        .empty(FIFO_empty[3]),
        .out(FIFO_out[3])
       
    );
 
    FIFO#(
        .FIFO_depth(InterSwitchBufferDepth),
        .FIFO_width(DataWidth)
    )
    FIFO_xneg(
        .clk(clk),
        .rst(rst),
        .in(in[4]),
        .produce(~in_pipeline_stall[4] && in[4][DataWidth-1]),
        .consume(FIFO_consume[4]),
        .full(FIFO_full[4]),
        .empty(FIFO_empty[4]),
        .out(FIFO_out[4])
       
    );
 
    FIFO#(
        .FIFO_depth(InterSwitchBufferDepth),
        .FIFO_width(DataWidth)
    )
    FIFO_zpos(
        .clk(clk),
        .rst(rst),
        .in(in[5]),
        .produce(~in_pipeline_stall[5] && in[5][DataWidth-1]),
        .consume(FIFO_consume[5]),
        .full(FIFO_full[5]),
        .empty(FIFO_empty[5]),
        .out(FIFO_out[5])
       
    );

    FIFO#(
        .FIFO_depth(InterSwitchBufferDepth),
        .FIFO_width(DataWidth)
    )
    FIFO_zneg(
        .clk(clk),
        .rst(rst),
        .in(in[6]),
        .produce(~in_pipeline_stall[6] && in[6][DataWidth-1]),
        .consume(FIFO_consume[6]),
        .full(FIFO_full[6]),
        .empty(FIFO_empty[6]),
        .out(FIFO_out[6])
    );

    assign priority[0]=FIFO_out[0][PriorityPos+PriorityWidth-1:PriorityPos];
    assign priority[1]=FIFO_out[1][PriorityPos+PriorityWidth-1:PriorityPos];
    assign priority[2]=FIFO_out[2][PriorityPos+PriorityWidth-1:PriorityPos];
    assign priority[3]=FIFO_out[3][PriorityPos+PriorityWidth-1:PriorityPos];
    assign priority[4]=FIFO_out[4][PriorityPos+PriorityWidth-1:PriorityPos];
    assign priority[5]=FIFO_out[5][PriorityPos+PriorityWidth-1:PriorityPos];
    assign priority[6]=FIFO_out[6][PriorityPos+PriorityWidth-1:PriorityPos];


    always@(*) begin
        if(priority[0]>=priority[1]) begin
            priority01=priority[0];
            sel_index01=0;
        end
        else begin
            priority01=priority[1];
            sel_index01=1;
        end
    end

    always@(*) begin
        if(priority[2]>=priority[3]) begin
            priority23=priority[2];
            sel_index23=2;
        end
        else begin
            priority23=priority[3];
            sel_index23=3;
        end
    end

    always@(*) begin
        if(priority[4]>=priority[5]) begin
            priority45=priority[4];
            sel_index45=4;
        end
        else begin
            priority45=priority[5];
            sel_index45=5;
        end
    end
    
    always@(*) begin
        if(priority01>=priority23) begin
            priority0123=priority01;
            sel_index0123=sel_index01;
        end
        else begin
            priority0123=priority23;
            sel_index0123=sel_index23;
        end
    end

    always@(*) begin
        if(priority45>=priority[6]) begin
            priority456=priority45;
            sel_index456=sel_index45;
        end
        else begin
            priority456=priority[6];
            sel_index456=6;
        end
    end

    always@(*) begin
        if(priority0123>=priority456) begin
            priority0123456=priority0123;
            sel_index0123456=sel_index0123;
        end
        else begin
            priority0123456=priority456;
            sel_index0123456=sel_index456;
        end
    end

    always@(*) begin
        if(sel_index0123456==0) begin
            FIFO_consume[0]=1;
            FIFO_consume[1]=0;
            FIFO_consume[2]=0;
            FIFO_consume[3]=0;
            FIFO_consume[4]=0;
            FIFO_consume[5]=0;
            FIFO_consume[6]=0;
        end
        else if(sel_index0123456==1) begin
            FIFO_consume[0]=0;
            FIFO_consume[1]=1;
            FIFO_consume[2]=0;
            FIFO_consume[3]=0;
            FIFO_consume[4]=0;
            FIFO_consume[5]=0;
            FIFO_consume[6]=0;
        end
        else if(sel_index0123456==2) begin
            FIFO_consume[0]=0;
            FIFO_consume[1]=0;
            FIFO_consume[2]=1;
            FIFO_consume[3]=0;
            FIFO_consume[4]=0;
            FIFO_consume[5]=0;
            FIFO_consume[6]=0;
        end
        else if(sel_index0123456==3) begin
            FIFO_consume[0]=0;
            FIFO_consume[1]=0;
            FIFO_consume[2]=0;
            FIFO_consume[3]=1;
            FIFO_consume[4]=0;
            FIFO_consume[5]=0;
            FIFO_consume[6]=0;
        end
        else if(sel_index0123456==4) begin
            FIFO_consume[0]=0;
            FIFO_consume[1]=0;
            FIFO_consume[2]=0;
            FIFO_consume[3]=0;
            FIFO_consume[4]=1;
            FIFO_consume[5]=0;
            FIFO_consume[6]=0;
        end
        else if(sel_index0123456==5) begin
            FIFO_consume[0]=0;
            FIFO_consume[1]=0;
            FIFO_consume[2]=0;
            FIFO_consume[3]=0;
            FIFO_consume[4]=0;
            FIFO_consume[5]=1;
            FIFO_consume[6]=0;
        end
        else if(sel_index0123456==6) begin
            FIFO_consume[0]=0;
            FIFO_consume[1]=0;
            FIFO_consume[2]=0;
            FIFO_consume[3]=0;
            FIFO_consume[4]=0;
            FIFO_consume[5]=0;
            FIFO_consume[6]=1;
        end
        else begin
            FIFO_consume[0]=0;
            FIFO_consume[1]=0;
            FIFO_consume[2]=0;
            FIFO_consume[3]=0;
            FIFO_consume[4]=0;
            FIFO_consume[5]=0;
            FIFO_consume[6]=0;
        end
    end


    always@(posedge clk) begin
        sel_data<=FIFO_out[sel_index0123456];
    end

//second stage read the reduction table entry if the packets is a reduction packet
    always@(posedge clk) begin
        sel_data_RR<=sel_data;
    end

    
    assign is_reduction=sel_data[ReductionBitPos];
    

    


    always@(posedge clk) begin
        if(is_reduction) begin
            //if the table entry about to be read is the same with the table entry about to be written, directly get the new one
            if(is_reduction_WB && sel_data[IndexWidth+IndexPos-1:IndexPos]==sel_data_RR[IndexWidth+IndexPos-1:IndexPos]) begin
                reduction_table_entry<=reduction_table_entry_next;
            end
            else begin
                reduction_table_entry<=reduction_table[sel_data[IndexWidth+IndexPos-1:IndexPos]];
            end
        end
    end

//third stage:
//
    

    assign next_counter=reduction_table_entry[158:156]+1;
    assign is_reduction_WB=sel_data_RR[ReductionBitPos];
    assign reduction_ready= sel_data_RR[ReductionBitPos] && (reduction_table_entry[ReductionTableWidth-9:ReductionTableWidth-11]==reduction_table_entry[ReductionTableWidth-12:ReductionTableWidth-14]+1);
    assign reduction_out={sel_data_RR[DataWidth-1:152],reduction_table_entry_next[135:128],reduction_table_entry_next[151:136],reduction_table_entry_next[127:0]};
     assign reduction_table_entry_next={reduction_out[DstPacketIDPos+7:DstPacketIDPos],reduction_table_entry[161:159],next_counter,reduction_table_entry[155:136],(reduction_table_entry[135:128]+sel_data_RR[WeightPos+WeightWidth-1:WeightPos]),(reduction_table_entry[PayloadLen-1:0]+sel_data_RR[PayloadLen-1:0])};


    always@(posedge clk) begin
        if(~is_reduction_WB) begin
            out<=sel_data_RR;
        end
        else if(reduction_ready) begin
            out<=reduction_out;
        end
        else begin
            out<=0;
        end
    end

    assign send=out[DataWidth-1];

    always@(posedge clk) begin
        if(is_reduction_WB) begin
            reduction_table[sel_data_RR[IndexWidth+IndexPos-1:IndexPos]]<=reduction_table_entry_next;
        end
    end
            


    





endmodule
        
        



    
    
    











        
    



