# IAP项目和python上位机联调Debug

## CRC校验问题

### 握手帧CRC16/数据帧CRC16校验失败

【原因分析】：上位机CRC代码有误，关于数据帧CRC16校验方式，校验内容调整为和STM32侧保持一致。
确定为`len+seq+payload`三个字段的CRC校验，而`data_frame_t`前面的`sof`和`type`字段用来界定`帧边界`，无需加入`CRC校验`(CRC校验保护有用的值，这样才有意义)

在测试的时候注意debug的`watch`和`call stack`调试窗口的使用，查看变量的值和调用函数链。在测试状态机的时候可以加入`串口发送逻辑`和`状态下if分支前后打断点`进行判断。

下一次还需要python跑上位机了记得要把具体的`CRC/自定义协议`协商好，不然最后查出来的还是上位机的问题，会浪费很多时间，注意对python代码的理解与学习，先调好`上位机`。

## 逻辑问题

### 应答帧设计

之前的应答帧只留给`数据帧`使用，在数据帧写入falsh以后发送ACK.

现在修改成`frame_ready`字段等于`true`之后(`即当前帧CRC校验成功以后发送ACK（握手帧）`)或者（`CRC成功然后写入flash之后发送ACK（数据帧）`）上位机需要确认握手帧发送成功以后再发送数据帧pkt,**即握手帧的响应帧用来界定和告知上位机可以发送PKT了。**

```c
#define FRAME_TYPE_HANDSHAKE    0x10
#define FRAME_TYPE_ACK_HS       0x11//add:握手帧应答帧类型
#define FRAME_TYPE_NACK_HS      0x12//（用于构成响应帧的type字段）
#define FRAME_TYPE_PKT          0x20//数据帧应答帧
#define FRAME_TYPE_ACK          0x80
#define FRAME_TYPE_NACK         0x81
```

## 上位机

*注意上位机通过串口和MCU通信，联调的时候`运行代码直接和单片机的串口连接`了，不要再把串口调试助手打开，这样会访问失败*

## STM32侧代码规范

***✅需要考虑大小端模式的情况就是出现了>1byte的数据类型，因为物理地址内存存储的区别（大小端模式），所以最好的方法是把那些变量换成uint8_t数组，这样就无需考虑大小端模式的差异了***

**和上位机通过串口通信**最好还是把数据类型换成`uint8_t`类型的二进制流比较好，可以避免大小端的问题。

**大端模式**：人的直觉/上位机都是大端模式，**高字节在前，低字节在后**。

**小端模式**：绝大多数`32位单片机`都是默认`小端模式`，**低字节在前，高字节在后**

解析的时候是没有大小端问题的，就是读取的时候会有问题，因为读取是通过`地址`和`物理内存`读取的,**一个地址对应一个字节**，调用读取的接口一般都是传入`起始地址`和`长度`,背后就是一套`increment`的**自增逻辑**。当按最小单位（字节/uint8_t）读取的时候当然会有问题,读取出来的**数据流是低字节在前，高字节在后**，这样数据流内部顺序混乱了，`CRC校验`也就会出错了。

### 🌟🛠️解决方法（以uint16_t类型为例）

#### 修改STM32的拼帧逻辑

1️⃣拼帧解析的时候故意把接收到的【高字节】【低字节】形式的两个uint8_t类型的数据拼凑成uint16_t数据，把`低字节<<8`|`高字节`,这样解析的时候就把存储顺序设置好了，读取的时候自然的原高字节在前，原低字节在后，不会有问题，但是这样语义就没有那么清晰了。

2️⃣在`STM32`CRC校验的时候调用接口也需要用到`起始地址`和`长度`，但是可以专门写一个`专属的CRC校验函数`，把**非uint8_t类型**的`帧成员变量`当作参数传进来，统一放在CRC校验函数体前面，先把这些参数拆成正确顺序的`uint8_t`类型以后先存入CRC校验缓冲区数组，最后再把payload放进去。

🛠️代码示例

```c
/**
* @brief  验证接收到的数据包 CRC16 是否正确(此函数内部将uint16_t成功转换成了二进制流)
 * @param  payload: 接收到的真货物缓冲区
 * @param  len: payload的长度
 * @param  seq: 包序号
 * @param  received_crc: 从串口最后 2 个字节读出来的期望 CRC
 * @return bool: true 校验成功，false 校验失败（数据被震歪了）
 */
bool APP_Ex_CRC16_Verify(const uint8_t *payload, uint16_t len, uint16_t seq, uint16_t received_crc)
{
    // 1. 创建一个临时校验缓冲区，把要校验的字段拼起来
    // 为什么要加 4？因为要算上 SEQ(2字节) 和 LEN(2字节)
    uint8_t verify_buf[512 + 4]; 
    uint16_t index = 0;

    // 2. 把 SEQ 塞进去（高字节在前）
    verify_buf[index++] = (uint8_t)(seq >> 8);
    verify_buf[index++] = (uint8_t)(seq & 0xFF);

    // 3. 把 LEN 塞进去（高字节在前）
    verify_buf[index++] = (uint8_t)(len >> 8);
    verify_buf[index++] = (uint8_t)(len & 0xFF);

    // 4. 把真正的 PAYLOAD 货物塞进去
    for(uint16_t i = 0; i < len; i++)
    {
        verify_buf[index++] = payload[i];
    }

    // 5. 现场算一遍整包的本生 CRC
    uint16_t calculated_crc = APP_Compute_CRC16(verify_buf, index);

    // 6. 对比结果
    if (calculated_crc == received_crc)
    {
        return true;  // 封条完好，货物安全！
    }
    else
    {
        return false; // 封条破损，拒绝接收！
    }
}

```

#### 上位机侧的代码逻辑

修改上位机侧的CRC校验逻辑,去配合STM32的小端模式

#### 最推荐:🌟🌟🌟通信的尽头还是uint8_t流啊