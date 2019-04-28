; library of math functions

.sin:
        addi x0, x0, 0  ; NOP caught by gpuemu; sin functionality will be proxied
        jalr x0, ra, 0                
