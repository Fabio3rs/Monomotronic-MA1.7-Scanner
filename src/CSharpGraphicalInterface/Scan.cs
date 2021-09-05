using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Monomotronic_MA1._7_Scanner
{
    public partial class Scan : Form
    {
        public Scan()
        {
            InitializeComponent();
            timer1.Start();
        }

        private void watertemp_CheckedChanged(object sender, EventArgs e)
        {
            Form1.setSensorReadState("watertemp", watertemp.Checked);
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            var watch = System.Diagnostics.Stopwatch.StartNew();
            if (watertemp.Checked)
            {
                String str = Form1.setSensorReadState("watertemp", watertemp.Checked).ToString();
                watertemp.Text = "Temperatura da água " + str + "ºC";
            }

            {
                String str = Form1.getThreadError().ToString();
                textBox1.Text = str;
            }

            /*{
                String str = Form1.getECUResponseStr();

                datar.Text = "Test\n" + str;
            }*/
            watch.Stop();
            var elapsedMs = watch.ElapsedMilliseconds;
            textBox1.Text += " " + elapsedMs.ToString();
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {

        }

        private void lererros_Click(object sender, EventArgs e)
        {
            Form1.setScanRequest(0);
        }

        private void cleanerrors_Click(object sender, EventArgs e)
        {
            Form1.setScanRequest(1);
        }

        private void datar_TextChanged(object sender, EventArgs e)
        {

        }
    }
}
