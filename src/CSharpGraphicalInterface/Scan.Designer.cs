namespace Monomotronic_MA1._7_Scanner
{
    partial class Scan
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
            this.cleanerrors = new System.Windows.Forms.Button();
            this.watertemp = new System.Windows.Forms.CheckBox();
            this.datar = new System.Windows.Forms.RichTextBox();
            this.lererros = new System.Windows.Forms.Button();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.tableLayoutPanel1.SuspendLayout();
            this.tableLayoutPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.Controls.Add(this.tableLayoutPanel2, 0, 1);
            this.tableLayoutPanel1.Controls.Add(this.textBox1, 0, 0);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 2;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 5.777778F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 94.22222F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(800, 450);
            this.tableLayoutPanel1.TabIndex = 0;
            // 
            // tableLayoutPanel2
            // 
            this.tableLayoutPanel2.ColumnCount = 2;
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel2.Controls.Add(this.cleanerrors, 1, 1);
            this.tableLayoutPanel2.Controls.Add(this.watertemp, 0, 0);
            this.tableLayoutPanel2.Controls.Add(this.datar, 0, 2);
            this.tableLayoutPanel2.Controls.Add(this.lererros, 1, 0);
            this.tableLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel2.Location = new System.Drawing.Point(3, 28);
            this.tableLayoutPanel2.Name = "tableLayoutPanel2";
            this.tableLayoutPanel2.RowCount = 3;
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 334F));
            this.tableLayoutPanel2.Size = new System.Drawing.Size(794, 419);
            this.tableLayoutPanel2.TabIndex = 1;
            // 
            // cleanerrors
            // 
            this.cleanerrors.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cleanerrors.Location = new System.Drawing.Point(400, 45);
            this.cleanerrors.Name = "cleanerrors";
            this.cleanerrors.Size = new System.Drawing.Size(391, 36);
            this.cleanerrors.TabIndex = 5;
            this.cleanerrors.Text = "Apagar erros";
            this.cleanerrors.UseVisualStyleBackColor = true;
            this.cleanerrors.Click += new System.EventHandler(this.cleanerrors_Click);
            // 
            // watertemp
            // 
            this.watertemp.AutoSize = true;
            this.watertemp.Location = new System.Drawing.Point(3, 3);
            this.watertemp.Name = "watertemp";
            this.watertemp.Size = new System.Drawing.Size(128, 17);
            this.watertemp.TabIndex = 0;
            this.watertemp.Text = "Temperatura da água";
            this.watertemp.UseVisualStyleBackColor = true;
            this.watertemp.CheckedChanged += new System.EventHandler(this.watertemp_CheckedChanged);
            // 
            // datar
            // 
            this.datar.Dock = System.Windows.Forms.DockStyle.Fill;
            this.datar.Location = new System.Drawing.Point(3, 87);
            this.datar.Name = "datar";
            this.datar.Size = new System.Drawing.Size(391, 329);
            this.datar.TabIndex = 1;
            this.datar.Text = "AAAAA";
            this.datar.TextChanged += new System.EventHandler(this.datar_TextChanged);
            // 
            // lererros
            // 
            this.lererros.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lererros.Location = new System.Drawing.Point(400, 3);
            this.lererros.Name = "lererros";
            this.lererros.Size = new System.Drawing.Size(391, 36);
            this.lererros.TabIndex = 2;
            this.lererros.Text = "Ler erros";
            this.lererros.UseVisualStyleBackColor = true;
            this.lererros.Click += new System.EventHandler(this.lererros_Click);
            // 
            // textBox1
            // 
            this.textBox1.Location = new System.Drawing.Point(3, 3);
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(560, 20);
            this.textBox1.TabIndex = 2;
            this.textBox1.TextChanged += new System.EventHandler(this.textBox1_TextChanged);
            // 
            // timer1
            // 
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // Scan
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 450);
            this.Controls.Add(this.tableLayoutPanel1);
            this.Name = "Scan";
            this.Text = "Scan";
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.tableLayoutPanel2.ResumeLayout(false);
            this.tableLayoutPanel2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel2;
        private System.Windows.Forms.CheckBox watertemp;
        private System.Windows.Forms.Timer timer1;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.RichTextBox datar;
        private System.Windows.Forms.Button cleanerrors;
        private System.Windows.Forms.Button lererros;
    }
}