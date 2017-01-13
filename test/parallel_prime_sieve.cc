#include <stdio.h>
#include <cmath>
extern "C" {
    #include "include/mcbsp.hpp"
    #include "include/mcbsp-affinity.h"
};
using namespace std;



int n_cores;
int bound;
int n_queries;
int mini[1000];
int maxi[1000];

void bspsieve()
{
    
    bsp_begin(n_cores);
    if (bound <= 36){ bound = 37;}
    int constant = (bound+n_cores)/n_cores;
    int my_pid = bsp_pid();
    int priem = 0;
    int primearray[constant];
    primearray[0]=0;
    bool numbers[constant];
    int primesum[n_cores-1];
    int resultmini[n_queries];
    int resultmaxi[n_queries];
    bsp_push_reg( &resultmini, sizeof(int)*(n_queries) );
    bsp_push_reg( &resultmaxi, sizeof(int)*(n_queries) );
    for (int i=0; i<(n_cores-1); i++){
        primesum[i]=0;
    }
    bsp_push_reg( &primesum, sizeof(int)*(n_cores-1) );
    bsp_push_reg( &priem, sizeof(int) );
    bsp_sync();
    
    if (my_pid == 0){
        numbers[0] = false;
        numbers[1] = false; 
        primearray[1]=0;
        for (int i=2; i<constant; i++){
            numbers[i] = true;
        }
        for (int i=2; i< sqrt(bound)+1; i++){
            primearray[i]=primearray[i-1];
            if (numbers[i] == true){
                primearray[i]++;
                priem = i;
                for (int j = 1; j<n_cores; j++){
                    bsp_put(j, &priem, &priem, 0, sizeof(int));
                }    
                bsp_sync();
                int help = i*2;
                while (help < constant){
                    numbers[help] = false;
                    help += i;
                }
                      
         
            }
         

        }
        
        priem = -1;
        for (int j=1; j<n_cores; j++){
            bsp_put(j, &priem, &priem, 0, sizeof(int));
        }
        bsp_sync();
        
        for (int i= sqrt(bound)+1; i<constant; i++){
            primearray[i]=primearray[i-1];
            if (numbers[i] == true){
                primearray[i]++;
            }
        }
        
        primesum[0]=primearray[constant-1];
        for(int i=1; i<n_cores; i++){
           bsp_put(i, &primesum[0] , &primesum, 0, sizeof(int)); 
        }
        
        
        bsp_sync();
        
        for(int i=1; i<constant; i++){
            if(numbers[i]==true){
                primearray[i]--;
            }
        }
        
        for(int i=0; i<n_queries; i++){
            if(mini[i]<constant){
                resultmini[i] = primearray[mini[i]];
            } 
            if(maxi[i]<constant){
                resultmaxi[i] = primearray[maxi[i]];
            }
        }
        
        bsp_sync();
        
        for (int i=0; i<n_queries; i++){
           printf("%d \n", resultmaxi[i]-resultmini[i]); 
        }
        
    }
    
    
    else{
        for (int i=0; i< constant; i++){
            numbers[i] = true;
        }
        
        bsp_sync();
        while (priem >= 0){
            int help1 = priem - (my_pid * constant) % priem;
            if (help1 == priem){
                help1 = 0;
            }
            while (help1 < constant){
                numbers[help1] = false;
                help1 += priem;
            }
            bsp_sync();
        }
        
        if(numbers[0]==true){
            primearray[0]=1;
        }
        for (int i=1; i<constant; i++){
            primearray[i]=primearray[i-1];
            if(numbers[i]==true){
                primearray[i]++;
            }
        }
        primesum[my_pid]=primearray[constant-1];
        for (int i=(my_pid+1); i<n_cores; i++){
           
            bsp_put(i, &primesum[my_pid] , &primesum, my_pid*sizeof(int), sizeof(int));
            
        }
        bsp_sync(); 
        
        int sum = 0;
        
        for (int i=0; i<my_pid; i++){
            sum+=primesum[i]; 
        }
        
        for(int i=0; i<constant; i++){
            primearray[i]+=sum;
        }
        
        for(int i=0; i<constant; i++){
            if(numbers[i]==true){
                primearray[i]--;
            }
        }
        
        for(int i=0; i<n_queries; i++){
            if(mini[i]>= (my_pid*constant) && mini[i]<((my_pid+1)*constant)){
                resultmini[i]=primearray[mini[i]-my_pid*constant];  
                bsp_put(0, &resultmini[i], &resultmini, i*sizeof(int), sizeof(int));
            }
            if(maxi[i]>= (my_pid*constant) && maxi[i]<((my_pid+1)*constant)){
                resultmaxi[i]=primearray[maxi[i]-my_pid*constant];  
                bsp_put(0, &resultmaxi[i], &resultmaxi, i*sizeof(int), sizeof(int));
            }               
        }
        bsp_sync();
                     
    }
    


    bsp_end();
}

int main(int argc, char* argv[])
{
    bsp_init(bspsieve, argc, argv);
	mcbsp_set_affinity_mode(COMPACT);

    scanf("%d",&n_cores);
	//if (n_cores > 4){n_cores = 4;}
    scanf("%d",&bound);
    scanf("%d",&n_queries); 
    for (int i=0; i<n_queries; i++){
        scanf("%d", &mini[i]);
        scanf("%d", &maxi[i]);
    }

    bspsieve();

    return 0;
}
