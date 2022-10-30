#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
int input[2] = {1, 2}; 

void *child(void *arg) {
   int *input = (int *) arg; // 取得資料
   int *result = (int *) malloc(sizeof(int) * 1); // 配置記憶體
   result[0] = input[0] + input[1]; // 進行計算
   pthread_exit((void *) result); // 傳回結果
}

// 主程式
int main() {
   pthread_t t1;
   // 建立子執行緒1，傳入 input 進行計算
   pthread_create(&t1, NULL, child, (void*) input);

   pthread_t t2;
   // 建立子執行緒2，傳入 input 進行計算
   pthread_create(&t2, NULL, child, (void*) input);


   void *ret; // 子執行緒傳回值
   // 等待子執行緒計算完畢
   pthread_join(t1, &ret);
   int *result_1 = (int *) ret;

   // 輸出計算結果
   printf("%d + %d = %d\n", input[0], input[1], result_1[0]);
   // 釋放記憶體
   free(result_1);


   pthread_join(t2, &ret);
   int *result_2 = (int *) ret;

   // 輸出計算結果
   printf("%d + %d = %d\n", input[0], input[1], result_2[0]);
   // 釋放記憶體
   free(result_2);
   

   

   return 0;
}