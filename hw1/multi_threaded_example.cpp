#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// 子執行緒函數
void *child(void *arg) {
   int *input = (int *) arg; // 取得資料
   int *result = (int *) malloc(sizeof(int) * 1); // 配置記憶體
   result[0] = input[0] + input[1]; // 進行計算
   pthread_exit((void *) result); // 傳回結果
}

// 主程式
int main() {
   pthread_t t;
   void *ret; // 子執行緒傳回值
   int input[2] = {1, 2}; // 輸入的資料

   // 建立子執行緒，傳入 input 進行計算
   pthread_create(&t, NULL, child, (void*) input);

   // 等待子執行緒計算完畢
   pthread_join(t, &ret);

   // 取得計算結果
   int *result = (int *) ret;

   // 輸出計算結果
   printf("%d + %d = %d\n", input[0], input[1], result[0]);

   // 釋放記憶體
   free(result);

   return 0;
}