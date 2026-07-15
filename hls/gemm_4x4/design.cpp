void matmul_4x4x4(float arg0[4][4], float arg1[4][4], float arg2[4][4]) {
#pragma HLS ARRAY_PARTITION variable=arg0 complete dim=0
#pragma HLS ARRAY_PARTITION variable=arg1 complete dim=0
#pragma HLS ARRAY_PARTITION variable=arg2 complete dim=0
  for (int i = 0; i < 4; i++) {
    #pragma HLS UNROLL
    for (int j = 0; j < 4; j++) {
      #pragma HLS UNROLL
      float acc = arg2[i][j];
      for (int k = 0; k < 4; k++) {
        #pragma HLS PIPELINE II=1
        acc = arg0[i][k] * arg1[k][j] + acc;
      }
      arg2[i][j] = acc;
    }
  }
}

