using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO.Ports;
using System.Runtime.InteropServices;

namespace Monomotronic_MA1._7_Scanner
{
    public partial class Form1 : Form
    {
        DataTable dt = new DataTable("COM PORTS");

        [DllImport("MonomotronicMA17.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool initECUManager(string port);

        [DllImport("MonomotronicMA17.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool isPortOpen();

        [DllImport("MonomotronicMA17.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool initECU();

        [DllImport("MonomotronicMA17.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern string getECUWelcome();

        [DllImport("MonomotronicMA17.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern string getECUResponseStr();

        [DllImport("MonomotronicMA17.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern double setSensorReadState(string sensor, bool state);

        [DllImport("MonomotronicMA17.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int getThreadError();

        [DllImport("MonomotronicMA17.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void setScanRequest(int val);

        private bool initCalled = false, welcomeMessage = false;

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            dt.Columns.Add(new DataColumn("name", typeof(string)));
            
            string[] ports = SerialPort.GetPortNames();
            
            foreach (string port in ports)
            {
                DataRow dr = dt.NewRow();

                dr["name"] = port;
                dt.Rows.Add(dr);
            }

            serialPort.DataSource = dt;
        }

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        private void connect_Click(object sender, EventArgs e)
        {
            serialPort.Enabled = false;
            connect.Enabled = false;

            initECUManager(serialPort.Text);

            //result.Text = "";

            if (isPortOpen())
            {
                connstatus.Text = "Porta serial aberta";
                toolStripProgressBar1.Value = 10;

                initECU();

                connstatus.Text = "Iniciando comunicação com o veículo";
                toolStripProgressBar1.Value = 15;
                initCalled = true;
                welcomeMessage = false;
                timer1.Start();
            }
            else
            {
                serialPort.Enabled = true;
                connect.Enabled = true;

                MessageBox.Show("Erro ao abrir a porta serial");
            }
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (initCalled)
            {
                if (!welcomeMessage)
                {
                    string str = getECUWelcome();

                    if (str.Length > 0)
                    {
                        welcomeMessage = true;
                        //result.Text += str;
                        toolStripProgressBar1.Value = 50;

                        timer1.Stop();
                        Scan s = new Scan();
                        s.Show();
                        connstatus.Text = "Conexão aberta";

                        MessageBox.Show(str, "Connected");
                        return;
                    }
                }

                int threrr = getThreadError();

                if (threrr != 0)
                {
                    connstatus.Text = "Erro do thread de conexão " + threrr.ToString();

                    serialPort.Enabled = true;
                    connect.Enabled = true;
                    toolStripProgressBar1.Value = 0;

                    initCalled = false;
                    toolStripProgressBar1.Value = 0;
                    MessageBox.Show("Conexão finalizada");

                    timer1.Stop();
                }
            }
        }
    }
}
