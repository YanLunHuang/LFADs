/**********
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

/*******************************************************************************
Description:
    HLS pragmas can be used to optimize the design : improve throughput, reduce latency and 
    device resource utilization of the resulting RTL code
    This is a wrapper to be used with an hls4ml project to enable proper handling by SDAccel
*******************************************************************************/
#include <iostream>
#include "myproject.h"
#include "kernel_params.h"

extern "C" {

void alveo_hls4ml(
    const group_in1 *in1, // Read-Only Vector
    const group_in2 *in2, // Read-Only Vector
    group_out *out       // Output Result
    )
{
    #pragma HLS INTERFACE m_axi port=in1 offset=slave bundle=gmem0
    #pragma HLS INTERFACE m_axi port=in2 offset=slave bundle=gmem1
    #pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem2
    #pragma HLS INTERFACE s_axilite port=in1 bundle=control
    #pragma HLS INTERFACE s_axilite port=in2 bundle=control
    #pragma HLS INTERFACE s_axilite port=out bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control
    
    #pragma HLS data_pack variable=in1
    #pragma HLS data_pack variable=in2
    #pragma HLS data_pack variable=out
    input_t  in_buf1[STREAM_LEN_IN1][STREAM_SIZE_IN1];
    #pragma HLS ARRAY_RESHAPE variable=in_buf1 complete dim=2
    input3_t in_buf2[STREAM_LEN_IN2][STREAM_SIZE_IN2];
    #pragma HLS ARRAY_RESHAPE variable=in_buf2 complete dim=2
    input3_t out_buf[STREAM_LEN_OUT][STREAM_SIZE_OUT];
    #pragma HLS ARRAY_RESHAPE variable=out_buf complete dim=2
    
    hls::stream<input_t> in_stream1[STREAM_SIZE_IN1];
    hls::stream<input3_t> in_stream2[STREAM_SIZE_IN2];
    hls::stream<result_t> out_stream[STREAM_SIZE_OUT];

    //If input or output variable is array
    //#pragma HLS ARRAY_PARTITION   variable=in_buf  complete dim=0
    //#pragma HLS ARRAY_PARTITION   variable=out_buf complete dim=0
    #pragma HLS STREAM variable=in_stream1 depth=73
    #pragma HLS STREAM variable=in_stream2 depth=73
    #pragma HLS STREAM variable=out_stream depth=73
    
  for (int k = 0; k < DATA_SET; k++){
    #pragma HLS DATAFLOW
    //Get data from DRAM
    for (int i = 0; i < STREAM_LEN_IN1; i++) {
        #pragma HLS PIPELINE II=1
        for (int j = 0; j < STREAM_SIZE_IN1; j++) {
            #pragma HLS UNROLL
            in_buf1[i][j] = in1[k*STREAM_LEN_IN1+i].layer[j];
        }
    }
    for (int i = 0; i < STREAM_LEN_IN2; i++) {
        #pragma HLS PIPELINE II=1
        for (int j = 0; j < STREAM_SIZE_IN2; j++) {
            #pragma HLS UNROLL
            in_buf2[i][j] = in2[k*STREAM_LEN_IN2+i].layer[j];
        }
    }
    //=============================================
    //Input
    //=============================================
    for(int i0 = 0; i0 < STREAM_LEN_IN1; i0++) {
        #pragma HLS PIPELINE II=1
        for(int i1 = 0; i1 < STREAM_SIZE_IN1; i1++) {
            #pragma HLS UNROLL
            input_t tmp = in_buf1[i0][i1];
            in_stream1[i1].write(tmp);
        }
    }
    for(int i0 = 0; i0 < STREAM_LEN_IN2; i0++) {
        #pragma HLS PIPELINE II=1
        for(int i1 = 0; i1 < STREAM_SIZE_IN2; i1++) {
            #pragma HLS UNROLL
            input3_t tmp2 = in_buf2[i0][i1];
            in_stream2[i1].write(tmp2);
        }
    }

    //=============================================
    //Start computation
    //=============================================

    std::cout<<"inf start"<<std::endl;
    myproject(in_stream1,in_stream2,out_stream);
    std::cout<<"inf end"<<std::endl;

    //=============================================
    //Output
    //=============================================
    for(int i0 = 0; i0 < STREAM_LEN_OUT; i0++) {
        #pragma HLS PIPELINE II=1
        for(int i1 = 0; i1 < STREAM_SIZE_OUT; i1++) { 
            #pragma HLS UNROLL
            result_t tmp3 = out_stream[i1].read();
            out_buf[i0][i1] = tmp3;
        }
    }

    for(int i0 = 0; i0 < STREAM_LEN_OUT; i0++) {
        #pragma HLS PIPELINE II=1
        for(int i1 = 0; i1 < STREAM_SIZE_OUT; i1++) { 
            #pragma HLS UNROLL
            out[k*STREAM_LEN_OUT+i0].layer[i1] = out_buf[i0][i1];
        }
    }
  }
}
}