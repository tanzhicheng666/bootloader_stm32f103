using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Threading;
using System.Threading.Tasks;

namespace BootloaderTool
{
    public class BootProtocol : IDisposable
    {
        private const byte Sof          = 0xAA;
        private const byte Eof          = 0x55;
        private const int  MaxFrameLen  = 255;
        private const int  WriteDataSize = 256;

        private const byte CmdPing       = 0x01;
        private const byte CmdPong       = 0x02;
        private const byte CmdErase      = 0x03;
        private const byte CmdEraseAck   = 0x04;
        private const byte CmdWriteAddr  = 0x05;
        private const byte CmdAck        = 0x06;
        private const byte CmdVerify     = 0x07;
        private const byte CmdVerifyAck  = 0x08;
        private const byte CmdReset      = 0x09;
        private const byte CmdWriteData  = 0x0A;

        private const byte StatusOk  = 0x00;
        private const byte StatusErr = 0x01;

        private readonly SerialPort _serialPort;
        private readonly SemaphoreSlim _semaphore = new SemaphoreSlim(1, 1);

        public bool IsOpen
        {
            get { return _serialPort != null && _serialPort.IsOpen; }
        }

        public BootProtocol(string portName, int baudRate = 115200)
        {
            _serialPort = new SerialPort(portName, baudRate, Parity.None, 8, StopBits.One)
            {
                ReadTimeout  = 100,
                WriteTimeout = 2000
            };
        }

        public void Open()
        {
            if (!_serialPort.IsOpen)
            {
                _serialPort.Open();
            }
        }

        public void Close()
        {
            if (_serialPort != null && _serialPort.IsOpen)
            {
                _serialPort.Close();
            }
        }

        public void Dispose()
        {
            _semaphore?.Dispose();
            _serialPort?.Dispose();
        }

        // ========================================================================
        // CRC-16/MODBUS
        // ========================================================================

        private static ushort Crc16(byte[] data, int offset, int length)
        {
            ushort crc = 0xFFFF;
            for (int i = 0; i < length; i++)
            {
                crc = (ushort)((crc >> 8) ^ Crc16Table[(crc ^ data[offset + i]) & 0xFF]);
            }
            return crc;
        }

        private static readonly ushort[] Crc16Table = {
            0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
            0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
            0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
            0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
            0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
            0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
            0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
            0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
            0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
            0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
            0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
            0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
            0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
            0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
            0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
            0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
            0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
            0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
            0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
            0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
            0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
            0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
            0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
            0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
            0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
            0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
            0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
            0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
            0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
            0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
            0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
            0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
        };

        // ========================================================================
        // Frame Send / Receive
        // ========================================================================

        /// <summary>
        /// 发送标准帧: SOF + Length + CMD + Data + CRC16 + EOF
        /// </summary>
        private async Task<(byte cmd, byte[] data)> SendStandardFrameAsync(
            byte cmd, byte[] payload, CancellationToken ct)
        {
            await _semaphore.WaitAsync(ct);
            try
            {
                int dataLen = payload?.Length ?? 0;
                byte length = (byte)(dataLen + 3);  // CMD + Data + CRC

                byte[] frame = new byte[length + 3]; // SOF + length_bytes + EOF
                frame[0] = Sof;
                frame[1] = length;
                frame[2] = cmd;

                if (dataLen > 0)
                {
                    Array.Copy(payload, 0, frame, 3, dataLen);
                }

                ushort crc = Crc16(frame, 2, dataLen + 1); // CRC over CMD + Data
                int pos = 3 + dataLen;
                frame[pos]     = (byte)(crc & 0xFF);
                frame[pos + 1] = (byte)((crc >> 8) & 0xFF);
                frame[pos + 2] = Eof;

                _serialPort.DiscardInBuffer();
                _serialPort.Write(frame, 0, frame.Length);

                return await ReceiveFrameAsync(ct);
            }
            finally
            {
                _semaphore.Release();
            }
        }

        /// <summary>
        /// 发送 WRITE_DATA 特殊帧: SOF + CMD(0x0A) + Data(256B) + CRC16 + EOF
        /// 无 Length 字节，固定 261 字节
        /// </summary>
        private async Task<(byte cmd, byte[] data)> SendWriteDataFrameAsync(
            byte[] block, CancellationToken ct)
        {
            await _semaphore.WaitAsync(ct);
            try
            {
                int totalLen = 1 + 1 + WriteDataSize + 2 + 1; // SOF + CMD + 256 + CRC + EOF
                byte[] frame = new byte[totalLen];

                frame[0] = Sof;
                frame[1] = CmdWriteData;
                Array.Copy(block, 0, frame, 2, WriteDataSize);

                ushort crc = Crc16(frame, 1, WriteDataSize + 1); // CRC over CMD + Data
                int pos = 2 + WriteDataSize;
                frame[pos]     = (byte)(crc & 0xFF);
                frame[pos + 1] = (byte)((crc >> 8) & 0xFF);
                frame[pos + 2] = Eof;

                _serialPort.DiscardInBuffer();
                _serialPort.Write(frame, 0, frame.Length);

                return await ReceiveFrameAsync(ct);
            }
            finally
            {
                _semaphore.Release();
            }
        }

        private async Task<(byte cmd, byte[] data)> ReceiveFrameAsync(CancellationToken ct)
        {
            DateTime deadline = DateTime.Now.AddMilliseconds(5000);
            List<byte> buffer = new List<byte>();

            while (DateTime.Now < deadline && !ct.IsCancellationRequested)
            {
                try
                {
                    int b = _serialPort.ReadByte();
                    if (b < 0)
                    {
                        continue;
                    }

                    buffer.Add((byte)b);

                    // 找 SOF
                    if (buffer.Count == 1 && buffer[0] != Sof)
                    {
                        buffer.Clear();
                        continue;
                    }

                    // 至少需要 SOF + Length + CMD + CRC(2) + EOF = 6 字节
                    if (buffer.Count >= 6 && buffer[buffer.Count - 1] == Eof)
                    {
                        // 解析 Length
                        byte length = buffer[1];

                        if (length < 3 || length > MaxFrameLen)
                        {
                            buffer.Clear();
                            continue;
                        }

                        int totalLen = length + 3; // SOF(1) + Length_field(1) + CMD_Data_CRC(length) + EOF(1)
                        if (buffer.Count < totalLen)
                        {
                            continue;
                        }

                        // 取完整帧
                        byte[] rawFrame = buffer.GetRange(0, totalLen).ToArray();
                        buffer.Clear();

                        // CRC 校验: 覆盖 CMD + Data = rawFrame[2 .. totalLen-3]
                        ushort rxCrc = (ushort)(rawFrame[totalLen - 3] | (rawFrame[totalLen - 2] << 8));
                        int crcLen = totalLen - 5; // minus SOF + length + CRC + EOF
                        ushort calcCrc = Crc16(rawFrame, 2, crcLen);

                        if (rxCrc == calcCrc)
                        {
                            byte rxCmd = rawFrame[2];
                            int dataLen = length - 3;
                            byte[] data = null;
                            if (dataLen > 0)
                            {
                                data = new byte[dataLen];
                                Array.Copy(rawFrame, 3, data, 0, dataLen);
                            }
                            return (rxCmd, data);
                        }

                        // CRC 不匹配，丢弃
                    }
                }
                catch (TimeoutException)
                {
                    continue;
                }
            }

            throw new TimeoutException("等待 Bootloader 回复超时");
        }

        // ========================================================================
        // Public API
        // ========================================================================

        /// <summary>
        /// PING 握手 — 确认 Bootloader 在线
        /// </summary>
        public async Task<bool> PingAsync(CancellationToken ct = default)
        {
            try
            {
                (byte cmd, _) = await SendStandardFrameAsync(CmdPing, null, ct);
                return cmd == CmdPong;
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// 擦除参数页
        /// </summary>
        public async Task<bool> EraseAsync(CancellationToken ct = default)
        {
            try
            {
                (byte cmd, byte[] data) = await SendStandardFrameAsync(CmdErase, null, ct);
                return cmd == CmdEraseAck && data != null && data.Length >= 1 && data[0] == StatusOk;
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// 设置 Flash 写入起始地址
        /// </summary>
        public async Task<bool> WriteAddrAsync(uint address, CancellationToken ct = default)
        {
            try
            {
                byte[] payload = new byte[4];
                payload[0] = (byte)(address & 0xFF);
                payload[1] = (byte)((address >> 8) & 0xFF);
                payload[2] = (byte)((address >> 16) & 0xFF);
                payload[3] = (byte)((address >> 24) & 0xFF);

                (byte cmd, byte[] data) = await SendStandardFrameAsync(CmdWriteAddr, payload, ct);
                return cmd == CmdAck && data != null && data.Length >= 1 && data[0] == StatusOk;
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// 写入 256 字节数据块 (地址需先通过 WriteAddrAsync 设置)
        /// </summary>
        public async Task<bool> WriteDataAsync(byte[] block, CancellationToken ct = default)
        {
            if (block == null || block.Length != WriteDataSize)
            {
                throw new ArgumentException(string.Format("数据块必须为 {0} 字节", WriteDataSize));
            }

            try
            {
                (byte cmd, byte[] data) = await SendWriteDataFrameAsync(block, ct);
                return cmd == CmdAck && data != null && data.Length >= 1 && data[0] == StatusOk;
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// 校验指定地址范围的 CRC-16/MODBUS
        /// payload = addr(4B LE) + len(4B LE) + expected_crc(2B LE) = 10 字节
        /// </summary>
        public async Task<bool> VerifyAsync(uint address, uint length, ushort expectedCrc,
            CancellationToken ct = default)
        {
            try
            {
                byte[] payload = new byte[10];
                payload[0] = (byte)(address & 0xFF);
                payload[1] = (byte)((address >> 8) & 0xFF);
                payload[2] = (byte)((address >> 16) & 0xFF);
                payload[3] = (byte)((address >> 24) & 0xFF);
                payload[4] = (byte)(length & 0xFF);
                payload[5] = (byte)((length >> 8) & 0xFF);
                payload[6] = (byte)((length >> 16) & 0xFF);
                payload[7] = (byte)((length >> 24) & 0xFF);
                payload[8] = (byte)(expectedCrc & 0xFF);
                payload[9] = (byte)((expectedCrc >> 8) & 0xFF);

                (byte cmd, byte[] data) = await SendStandardFrameAsync(CmdVerify, payload, ct);
                return cmd == CmdVerifyAck && data != null && data.Length >= 1 && data[0] == StatusOk;
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// 软件复位 MCU
        /// </summary>
        public async Task ResetAsync(CancellationToken ct = default)
        {
            await SendStandardFrameAsync(CmdReset, null, ct);
        }
    }
}
