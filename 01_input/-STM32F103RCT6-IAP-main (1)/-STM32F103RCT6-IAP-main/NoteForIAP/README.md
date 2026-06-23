# IAP项目Debug日志

## Overview概述

项目包含两个`keil工程`和一份`.bin`固件.
其中，含一个`bootloader`引导程序和一份`APP_A`运行固件。引导程序默认原烧录起始位置，修改`ROM`大小为`0x8000`(20KB)

```text
#bootloader 魔术棒 Target选项卡配置
1️⃣Start: 0x8000000
2️⃣Size:0x8000
🌟bootloader侧只需修改size，其他保持默认即可，无需设置中断向量表偏移。
```

## **bootloader工作流**

```text

1️⃣读flash:查看bootinformation分区（预留2kB：F103RCT6的最小擦除单位，page大小，1page/2kB）
⬇️
2️⃣OTA状态机根据读取到的bootinfo判断要运行的分区
⬇️
deinit所有初始化/使用的外设，关闭中断跳转app分区运行程序

```

## bootloder的deinit

**🕒 为什么“只加 DeInit”会崩溃的时序图**

```text
======================= ❌ 未初始化却执行 DeInit 的崩溃现场 =======================

Bootloader 启动 🔌
       |
       v
  HAL_Init()         -> 🟢 初始化了 SysTick 等基础时钟
       |
  [ 删除了 MX_USART1_UART_Init() ] -> ⚠️ 注意：此时 huart1 结构体内部全是垃圾随机值！
       |
       v
  HAL_UART_DeInit(&huart1)  🏃‍♂️ 执行反初始化...
       |
       +---> (1) 进入函数内部，HAL 库首先检查：if (huart == NULL) -> ❌ 不是空指针，继续
       |
       +---> (2) 准备复位硬件，执行关键底层宏：
       |         __HAL_UART_RESET_HANDLE_STATE(huart);
       |         
       +---> (3) 💥 致命触雷点 💥：
                 代码试图去读写：huart->Instance->BRR 或 huart->Instance->CR1
                 |
                 v 
  [ 此时 huart->Instance 里的值是 0x00000000 或者一堆随机乱码地址！ ]
                 |
                 v
  CPU 强行向非法/不存在的寄存器地址写入数据 🛑
                 |
                 v
  硬件拉响警报：触发 HardFault 🚨 (画面定格，死机！)
```

### 什么时候deinit

| 情况 | 后果 |
| --- | --- |
| 外设有中断使能且向量表已切换 | 中断触发时跑到 App 的 handler，但外设状态是 bootloader 的，逻辑错乱 |
| 外设有 DMA 且 EN=1 | App 重新配置 DMA 时写入被忽略，静默失败 |
| 外设占用了共享资源（时钟、GPIO复用） | App 初始化时状态不一致 |

---

## 跳转的时序和准备工作

```c
void jump_to_app(uint32_t app_addr)
{
    /* 1. 检查栈指针合法性 */
    uint32_t app_sp  = *(volatile uint32_t *)(app_addr);
    uint32_t app_pc  = *(volatile uint32_t *)(app_addr + 4);

    if ((app_sp & 0x2FFE0000) != 0x20000000) {  /* 简单合法性检查 */
        return;  /* App不存在或损坏 */
    }

    /* 2. DeInit 用到的外设 */
    HAL_CRC_DeInit(&hcrc);
    __HAL_RCC_CRC_CLK_DISABLE();

    /* 如果USART被初始化过 */
    HAL_UART_DeInit(&huart1);
    __HAL_RCC_USART1_CLK_DISABLE();

    /* 3. 关闭SysTick */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    /* 4. 关闭全局中断 */
    __disable_irq();

    /* 5. 重定向向量表到App（如果App不在0x08000000） */
    SCB->VTOR = app_addr;

    /* 6. 设置栈指针并跳转 */
    __set_MSP(app_sp);

    typedef void (*app_entry_t)(void);
    app_entry_t app_entry = (app_entry_t)app_pc;
    app_entry();
}
```

## **APP侧工作流**

**PRIMASK寄存器被置一，关闭全局中断，所以APP运行的时候，中断仍然是被关闭的，所以就不会产生中断**

*PRIMASK*：`priority mask register`,优先级屏蔽寄存器，位宽是1bit，为1关闭所有中断

当程序执行 JumpToApp(); 信仰一跃跳进 APP 之后，CPU 的寄存器状态是保留的。也就是说，这个总电闸在 APP 里面依然是【关闭】状态

在标准的 STM32 APP 里面，不管是 HAL_Init() 还是 MX_USART1_UART_Init()，**没有任何一个官方函数会自动帮你调用 __enable_irq();** 来重新开闸！ 官方默认你上电时电闸就是开的。

***bootloader侧关闭中断***

```c

__disable_irq(); // 关闭所有中断

//🔍⬆️注意这里的屏蔽中断是通过对PRIMASK寄存器置一实现的

```

***APP侧开启中断***

```c
int main(void)
{
  /* USER CODE BEGIN 1 */
  
  // 1. 设置中断向量表偏移（你之前加的）
  SCB->VTOR = FLASH_BASE | 0x8800; 
  
  // 2. 【极其关键】重新开启全局中断！把 Bootloader 关掉的电闸重新拉起来！
  __enable_irq(); 
  
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();
  
  // ... 后续外设初始化 ...
}
```


**🚨注意在bootloader跳转前关闭了所有中断，所以在main函数最前面一定要开启全局中断**


## APP侧烧录前的配置

### 烧录位置选择

魔术棒➡️Target➡️修改ROM的起始地址和大小

```text
start:0x8008800(APP_A自身分区的起始地址)
size:0x180000
```

### 中断向量表设置

**每个独立完整的固件都有自己的中断向量表，所以在修改固件偏移地址以后，中断向量表也不是默认的地址了，需要添加偏移量**

可以通过修改宏定义设置`中断向量表offset`修改偏移地址

```text
1️⃣Ctrl+F 全局搜索 #define VECT_TAB_OFFSET

⬇️进入system_stm32f1xx.c文件

2️⃣ 文件第96行：#define USER_VECT_TAB_ADDRESS ，去掉注释对

⬇️设置偏移地址（即使在跳转前设置了SCB->VTOR字段，但是执行reset handler的时候这个字段会恢复默认，
如果没有设置上面的宏，所以APP侧必须修改中断向量表偏移地址）

3️⃣#define VECT_TAB_OFFSET         0x00008800U     /*!< Vector Table base offset field.

```

#### bootloader侧和app侧两端都要设置中断向量表的偏移

```text
======================= ❌ 在 Bootloader 里提前设置 VTOR 的翻车现场 =======================

[ Bootloader 阶段 ]
  SCB->VTOR = 0x08008800;  -> 🟢 提前把向量表指向 APP
  bootloder_JumpToApp();   -> 🏃‍♂️ 信仰一跃，跳向 APP 的 Reset_Handler

---------------------------------------------------------------------------------------

[ APP 刚启动的瞬间（main 还没执行）]
  Reset_Handler
       |
       v
  调用 SystemInit() 
       |
       +---> 执行源码：SCB->VTOR = FLASH_BASE; (即 0x08000000)
       |
       v
  [ 🚨 惨剧发生：你提前设置的 0x08008800 在这里被 APP 自己无情洗掉了！ ]
       |
       v
  进入 APP 的 main() -> 执行你的 trans_init() -> 开启串口 DMA 接收
       |
       v
  电脑发送数据 `hello` ======> 🔔 触发串口空闲中断
       |
       +---> CPU 去读当前的 SCB->VTOR 寄存器。
       +---> 惊恐地发现：此时地址是 0x08000000 (回到了 Bootloader 的地盘)！
       +---> CPU 跑到 Bootloader 的老向量表里执行中断。
       |
       v
[ 最终结局：HardFault 🚨 ]
  由于 Bootloader 早已关闭或者代码错位，CPU 找不到正确的处理函数，当场暴毙死机。
```

#### 可以测试一下在Bootloader开启串口中断，在中断里面写回发逻辑，app这边不设置中断向量表地址，看看会不会执行回调函数，执行那边的回调函数

### Reset Handler内部

```text
======================= ⚙️ Reset_Handler 内部硬核流水线 =======================

Bootloader 信仰一跃 🏃‍♂️ -> 核心跳入 APP 的 Reset_Handler 入口
                             |
                             v
  【步骤 1：恢复堆栈和核心状态】
  * 确保当前 CPU 处于特权级线程模式，使用刚刚赋的值更新 SP（堆栈指针）。
                             |
                             v
  【步骤 2：调用 SystemInit()】 ---> 🚨 触发灯下黑的关键点！
  * 彻底复位 RCC 时钟寄存器（把时钟树重置回内部 HSI 默认值）。
  * 强行执行：SCB->VTOR = FLASH_BASE; (直接把你在 Bootloader 设的重定向给洗掉了！)
                             |
                             v
  【步骤 3：C 语言运行环境搭建（你猜对的部分 🟢）】
  * 把 Flash 里的 `.data` 段（有初值的全局变量）复制到 SRAM 中。
  * 把 SRAM 里的 `.bss` 段（没有初值的全局变量）全部清零。
                             |
                             v
  【步骤 4：调用 __main() 库函数】
  * 执行 C++ 的构造函数（如果有）。
  * 初始化 C 语言的 堆（Heap）和 栈（Stack）分配空间。
                             |
                             v
  【步骤 5：正义一跃，进入 main()】
  * 终于来到了你在 main.c 里写的第一行代码！
```

## 出现的问题

### 串口中断不能执行（可以调用transmit向电脑发信息）

1.APP侧只设置了target选项卡的start和size，没有设置中断向量表的偏移，导致找不到对应的`中断回调函数`的入口地址。

2.在`bootloader`侧cubemx配置了串口，但是没有在跳转之前`deinit`

3.APP侧烧录地址选错，选的APP_B起始地址，`hardfault`出错。

4.🌟在`bootloader`执行`新的reset handler`之前屏蔽了所有中断，`__disable_irq`,修改了`PRIMASK寄存器`，在执行`APP_A分区`的时候寄存器还是原来修改以后的状态，**依旧屏蔽中断**，所以导致即使`串口有数据过来`，但是中断根本就不能触发，回调函数自然也不能执行。

5.***🔍🚨今天晚上回去排查一下，之前握手帧解析那里显示rb空指针是怎么回事，按理说初始化的时候应该把指针送过去了啊，不知道为什么在解析的时候会出问题，rb_peek()非法访问***

```text
OTA任务切片
⬇️
WAIT_HS状态
⬇️
上位机发送数据
⬇️
进入串口中断回调，写入环形缓冲区
⬇️
主循环轮询（WAIT_HS+环形缓冲区非空）如果这个对应的rb是空指针，会返回false（即使没有上位机发送数据，看到false以为是缓冲区非空了）
⬇️
进入app_parser_hs()解析器
⬇️
解析器根据rb指针peek缓冲区内部的值，发现rb=NULL,非法访问，HardFault🔴
```

**破案了，是我把main里面的初始化换成传输层的初始化了，把app_ota_init()初始化给屏蔽了**


***Debug模式使用方法***

```text
进入debug模式
⬇️
RUN全速执行
⬇️
STOP，软件会自动跳转到当前执行的代码处，黄蓝双色箭头指向,点击view->call stack->查看函数调用链
```

---

## ST-LINK debug方法

### 🛠️ 核心招式一：看调用栈（Call Stack）—— 直接定位死在哪个函数

当程序停在 HardFault_IRQHandler 的 while(1) 时：

在 Keil 菜单栏点击 View -> Call Stack Window（或者在右下角/右上角找一个叫 Call Stack + Locals 的窗口）。

在这个窗口里，你会看到一串函数调用链。

最顶层一定是 HardFault_IRQHandler。

看它紧接着的下一行是什么函数！那个函数就是把程序送进地狱的“罪魁祸首”。双击那一行，Keil 会直接跳转到崩溃前执行的最后一行代码。

### 🛠️ 核心招式二：查看内核寄存器（抓取死因）

当发生 HardFault 时，内核会把当时的 CPU 状态记录在寄存器里。大容量芯片（RCT6）有专门的错误状态寄存器。

在 Debug 状态下，点击 Keil 左侧的 Registers 窗口（如果没有，点击 View -> Registers Window）。

展开 Banked Registers 或者直接看核心寄存器：

R0 ~ R3, R12：看看这些寄存器里有没有 0x00000000（空指针）或者一些很奇怪的超界地址（如 0x2000XXXX 越界）。

PC (Program Counter)：这是程序计数器。里面记录了一串十六进制地址（比如 0x0800A124）。

绝招：复制这个 PC 寄存器里的地址，然后在 Keil 的 Disassembly（反汇编） 窗口里右键选择 Go to Address...，输入这个地址。光标跳过去的地方，就是导致死机的最精准的那一条汇编指令！