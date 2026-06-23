using System;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace BootloaderTool
{
    public class MainForm : Form
    {
        // ====================================================================
        // UI Controls
        // ====================================================================
        private ComboBox   _cbPort;
        private Button     _btnRefresh;
        private Button     _btnOpenFile;
        private Button     _btnFlash;
        private ComboBox   _cbBaudrate;
        private Label      _lblStatus;
        private ProgressBar _progressBar;
        private TextBox    _txtLog;
        private TextBox    _txtHex;
        private Label      _lblFile;

        private string     _firmwarePath;
        private byte[]     _firmwareData;
        private CancellationTokenSource _cts;

        private const uint AppStartAddr = 0x08004800u;

        public MainForm()
        {
            InitializeComponent();
            RefreshPorts();
        }

        // ====================================================================
        // UI Setup
        // ====================================================================

        private void InitializeComponent()
        {
            Text = "STM32F1 Bootloader Tool";
            Size = new Size(820, 500);
            FormBorderStyle = FormBorderStyle.FixedSingle;
            MaximizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;
            Font = new Font("Consolas", 9f);

            Label lblPort = new Label
            {
                Text = "COM:", Location = new Point(15, 18), Size = new Size(40, 22)
            };
            _cbPort = new ComboBox
            {
                Location = new Point(55, 15), Size = new Size(90, 22),
                DropDownStyle = ComboBoxStyle.DropDownList
            };
            _btnRefresh = new Button
            {
                Text = "刷新", Location = new Point(150, 14), Size = new Size(50, 24)
            };
            _btnRefresh.Click += (s, e) => RefreshPorts();

            Label lblBaud = new Label
            {
                Text = "波特率:", Location = new Point(220, 18), Size = new Size(55, 22)
            };
            _cbBaudrate = new ComboBox
            {
                Location = new Point(275, 15), Size = new Size(80, 22),
                DropDownStyle = ComboBoxStyle.DropDownList
            };
            _cbBaudrate.Items.AddRange(new object[] { "9600", "19200", "38400", "57600", "115200" });
            _cbBaudrate.SelectedIndex = 4;

            _btnOpenFile = new Button
            {
                Text = "选择固件...", Location = new Point(15, 48), Size = new Size(90, 24)
            };
            _btnOpenFile.Click += OnOpenFile;
            _lblFile = new Label
            {
                Text = "未选择文件", Location = new Point(110, 50),
                Size = new Size(380, 22), ForeColor = Color.Gray
            };

            _btnFlash = new Button
            {
                Text = "开始下载", Location = new Point(15, 82), Size = new Size(90, 30),
                Enabled = false, BackColor = Color.SteelBlue, ForeColor = Color.White
            };
            _btnFlash.Click += OnFlash;

            _lblStatus = new Label
            {
                Text = "就绪", Location = new Point(15, 120),
                Size = new Size(480, 22), ForeColor = Color.Gray
            };

            _progressBar = new ProgressBar
            {
                Location = new Point(15, 145), Size = new Size(480, 22),
                Minimum = 0, Maximum = 100
            };

            Label lblHex = new Label
            {
                Text = "固件内容 (HEX):", Location = new Point(15, 175), Size = new Size(120, 18)
            };

            _txtHex = new TextBox
            {
                Location = new Point(15, 195), Size = new Size(380, 250),
                Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical,
                BackColor = Color.Black, ForeColor = Color.Lime,
                Font = new Font("Consolas", 8f), WordWrap = false
            };

            _txtLog = new TextBox
            {
                Location = new Point(410, 195), Size = new Size(390, 250),
                Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical,
                BackColor = Color.Black, ForeColor = Color.Lime,
                Font = new Font("Consolas", 8f)
            };

            Controls.AddRange(new Control[] {
                lblPort, _cbPort, _btnRefresh,
                lblBaud, _cbBaudrate,
                _btnOpenFile, _lblFile,
                _btnFlash, _lblStatus, _progressBar,
                lblHex, _txtHex, _txtLog
            });
        }

        private void RefreshPorts()
        {
            _cbPort.Items.Clear();
            foreach (string name in SerialPort.GetPortNames())
            {
                _cbPort.Items.Add(name);
            }
            if (_cbPort.Items.Count > 0)
            {
                _cbPort.SelectedIndex = 0;
            }
        }

        // ====================================================================
        // File Selection
        // ====================================================================

        private void OnOpenFile(object sender, EventArgs e)
        {
            using (OpenFileDialog dlg = new OpenFileDialog())
            {
                dlg.Filter = "Binary files (*.bin)|*.bin|Hex files (*.hex)|*.hex|All files (*.*)|*.*";
                dlg.Title = "选择固件文件";
                if (dlg.ShowDialog() == DialogResult.OK)
                {
                    _firmwarePath = dlg.FileName;
                    _lblFile.Text = Path.GetFileName(_firmwarePath);
                    _lblFile.ForeColor = Color.Black;

                    try
                    {
                        _firmwareData = File.ReadAllBytes(_firmwarePath);
                        Log(string.Format("加载固件: {0} 字节", _firmwareData.Length));
                        ShowHex(_firmwareData);
                        _btnFlash.Enabled = _cbPort.SelectedItem != null;
                    }
                    catch (Exception ex)
                    {
                        Log(string.Format("错误: {0}", ex.Message));
                        _btnFlash.Enabled = false;
                    }
                }
            }
        }

        // ====================================================================
        // Flash Download
        // ====================================================================

        private async void OnFlash(object sender, EventArgs e)
        {
            if (_firmwareData == null || _firmwareData.Length == 0)
            {
                MessageBox.Show("请先选择固件文件", "错误",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            if (_cbPort.SelectedItem == null)
            {
                MessageBox.Show("请选择 COM 口", "错误",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            string portName = _cbPort.SelectedItem.ToString();
            int baudRate = int.Parse(_cbBaudrate.SelectedItem.ToString());

            _btnFlash.Enabled = false;
            _btnOpenFile.Enabled = false;
            _progressBar.Value = 0;
            _cts = new CancellationTokenSource();

            try
            {
                await DownloadFirmwareAsync(portName, baudRate, _cts.Token);
            }
            catch (OperationCanceledException)
            {
                Log("== 下载已取消 ==");
                SetStatus("已取消", Color.Orange);
            }
            catch (Exception ex)
            {
                Log(string.Format("== 下载失败: {0} ==", ex.Message));
                SetStatus("下载失败", Color.Red);
            }
            finally
            {
                _btnFlash.Enabled = true;
                _btnOpenFile.Enabled = true;
                _cts?.Dispose();
                _cts = null;
            }
        }

        private async Task DownloadFirmwareAsync(string portName, int baudRate, CancellationToken ct)
        {
            using (BootProtocol proto = new BootProtocol(portName, baudRate))
            {
                proto.Open();

                // 1. 握手
                Log(">>> 握手...");
                SetStatus("握手中...", Color.Blue);

                if (!await proto.PingAsync(ct))
                {
                    throw new Exception("握手失败 — MCU 无响应，请确认串口连接和 Bootloader 运行中");
                }
                Log("<<< 握手成功");

                // 2. 擦除参数区
                Log(">>> 擦除参数区...");
                SetStatus("擦除 Flash...", Color.Orange);

                if (!await proto.EraseAsync(ct))
                {
                    throw new Exception("擦除失败");
                }
                Log("<<< 擦除完成");

                // 3. 设置写入起始地址
                Log(string.Format(">>> 设置写入地址: 0x{0:X8}...", AppStartAddr));
                if (!await proto.WriteAddrAsync(AppStartAddr, ct))
                {
                    throw new Exception("设置写入地址失败");
                }
                Log("<<< 地址已设置");

                // 4. 逐块写入
                int totalBlocks = (_firmwareData.Length + 255) / 256;
                Log(string.Format(">>> 开始写入 {0} 字节 ({1} 块)...",
                    _firmwareData.Length, totalBlocks));

                byte[] block = new byte[256];
                for (int i = 0; i < totalBlocks; i++)
                {
                    ct.ThrowIfCancellationRequested();

                    Array.Clear(block, 0, 256);
                    int copyLen = Math.Min(256, _firmwareData.Length - i * 256);
                    Array.Copy(_firmwareData, i * 256, block, 0, copyLen);

                    if (!await proto.WriteDataAsync(block, ct))
                    {
                        throw new Exception(string.Format(
                            "写入失败 @ 块 {0}/{1}", i + 1, totalBlocks));
                    }

                    int pct = (i + 1) * 100 / totalBlocks;
                    _progressBar.InvokeIfRequired(() => _progressBar.Value = pct);
                    SetStatus(string.Format("写入中... {0}/{1}", i + 1, totalBlocks), Color.Blue);
                }
                Log("<<< 写入完成");

                // 5. 校验
                Log(">>> 校验固件...");
                SetStatus("校验中...", Color.Orange);

                ushort crc = 0xFFFF;
                for (int i = 0; i < _firmwareData.Length; i++)
                {
                    crc = (ushort)((crc >> 8) ^ Crc16Table[(crc ^ _firmwareData[i]) & 0xFF]);
                }

                if (!await proto.VerifyAsync(AppStartAddr, (uint)_firmwareData.Length, crc, ct))
                {
                    throw new Exception("CRC 校验失败");
                }
                Log(string.Format("<<< 校验通过 (CRC-16 = 0x{0:X4})", crc));

                // 6. 复位
                Log(">>> 复位 MCU...");
                await proto.ResetAsync(ct);
                Log("<<< 复位完成");

                SetStatus("下载成功!", Color.Green);
                _progressBar.InvokeIfRequired(() => _progressBar.Value = 100);
                MessageBox.Show("固件下载成功!", "完成",
                    MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }

        // ====================================================================
        // CRC-16 Table (same as MCU)
        // ====================================================================
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

        // ====================================================================
        // Hex Viewer
        // ====================================================================

        private void ShowHex(byte[] data)
        {
            if (data == null || data.Length == 0)
            {
                _txtHex.Text = string.Empty;
                return;
            }

            int maxShow = Math.Min(data.Length, 65536); // show at most 64KB
            System.Text.StringBuilder sb = new System.Text.StringBuilder();

            for (int i = 0; i < maxShow; i += 16)
            {
                sb.AppendFormat("{0:X8}: ", AppStartAddr + i);

                for (int j = 0; j < 16; j++)
                {
                    if (j == 8)
                    {
                        sb.Append(' ');
                    }

                    if ((i + j) < maxShow)
                    {
                        sb.AppendFormat("{0:X2} ", data[i + j]);
                    }
                    else
                    {
                        sb.Append("   ");
                    }
                }

                sb.AppendLine();
            }

            if (data.Length > maxShow)
            {
                sb.AppendLine(string.Format("... (共 {0} 字节，仅显示前 64KB)", data.Length));
            }

            _txtHex.InvokeIfRequired(() => _txtHex.Text = sb.ToString());
        }

        // ====================================================================
        // UI Helpers
        // ====================================================================

        private void Log(string msg)
        {
            if (_txtLog.InvokeRequired)
            {
                _txtLog.BeginInvoke(new Action(() => Log(msg)));
                return;
            }
            string timestamp = DateTime.Now.ToString("HH:mm:ss");
            _txtLog.AppendText(string.Format("[{0}] {1}\r\n", timestamp, msg));
        }

        private void SetStatus(string msg, Color color)
        {
            if (_lblStatus.InvokeRequired)
            {
                _lblStatus.BeginInvoke(new Action(() => SetStatus(msg, color)));
                return;
            }
            _lblStatus.Text = msg;
            _lblStatus.ForeColor = color;
        }
    }

    /// <summary>
    /// Extension method for safe cross-thread ProgressBar update
    /// </summary>
    public static class ControlExtensions
    {
        public static void InvokeIfRequired(this Control control, Action action)
        {
            if (control.InvokeRequired)
            {
                control.BeginInvoke(action);
            }
            else
            {
                action();
            }
        }
    }
}
