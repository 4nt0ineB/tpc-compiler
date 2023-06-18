int x;

int function(int x, char y){
    int i;
    int z = 10;
    char c = '\68', c2 = '\115', c3 = z + 4;
    while(z != 0){
        x = x + y;
        z = z - 1;
        if(x % 10 == 0){
            x = 12;
        }else if(x <= 1){
            y = f1() + f2();
        }
    }
    c = c + 25;
    return x;
}  

int main(void){
    function(12, 'a');
    return 0;
}