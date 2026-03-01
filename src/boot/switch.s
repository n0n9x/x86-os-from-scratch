[GLOBAL switch_to_task]
switch_to_task:
    ; --- 1. 保存当前任务上下文 ---
    push ebx
    push esi
    push edi
    push ebp

    ; --- 2. 在切换栈之前，先取出所有需要的参数 ---
    mov eax, [esp + 20]     ; eax = old_esp_ptr
    mov edx, [esp + 24]     ; edx = new_esp
    mov ecx, [esp + 28]     ; ecx = new_cr3

    ; --- 3. 保存旧栈顶 ---
    mov [eax], esp          ; *old_esp_ptr = esp

    ; --- 4. 切换地址空间 (CR3) ---
    ; 关键顺序：先换栈，再换页表
    ; 因为新栈地址在两个页表里都有内核映射，这样最稳妥
    mov esp, edx
    mov eax, cr3
    cmp eax, ecx
    je .skip_cr3
    mov cr3, ecx        ; 切换页表，刷新 TLB
.skip_cr3:

    ; --- 5. 恢复新任务上下文 ---
    pop ebp
    pop edi
    pop esi
    pop ebx

    ret ; 弹出新任务栈上的返回地址

[GLOBAL first_return_from_kernel]
first_return_from_kernel:
    pop eax          ; 弹出栈上的 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    iret             ; 自动从栈帧恢复 EIP, CS, EFLAGS, ESP, SS