#pragma once

namespace WindowsForm {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for Form1
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		Form1(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::ComboBox^ comboBox1;
	protected:
	private: System::Windows::Forms::Button^ button1;




	private: System::IO::Ports::SerialPort^ serialPort1;

	private: System::Windows::Forms::ListBox^ listBox1;

	private: System::Windows::Forms::Timer^ timer1;


	private: System::ComponentModel::IContainer^ components;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			this->comboBox1 = (gcnew System::Windows::Forms::ComboBox());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->serialPort1 = (gcnew System::IO::Ports::SerialPort(this->components));
			this->listBox1 = (gcnew System::Windows::Forms::ListBox());
			this->timer1 = (gcnew System::Windows::Forms::Timer(this->components));
			this->SuspendLayout();
			// 
			// comboBox1
			// 
			this->comboBox1->FormattingEnabled = true;
			this->comboBox1->Location = System::Drawing::Point(12, 12);
			this->comboBox1->Name = L"comboBox1";
			this->comboBox1->Size = System::Drawing::Size(121, 21);
			this->comboBox1->TabIndex = 0;
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(148, 12);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(75, 23);
			this->button1->TabIndex = 1;
			this->button1->Text = L"button1";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &Form1::button1_Click);
			// 
			// serialPort1
			// 
			this->serialPort1->DataReceived += gcnew System::IO::Ports::SerialDataReceivedEventHandler(this, &Form1::serialPort1_DataReceived);
			// 
			// listBox1
			// 
			this->listBox1->FormattingEnabled = true;
			this->listBox1->Location = System::Drawing::Point(13, 39);
			this->listBox1->Name = L"listBox1";
			this->listBox1->Size = System::Drawing::Size(209, 108);
			this->listBox1->TabIndex = 3;
			// 
			// timer1
			// 
			this->timer1->Enabled = true;
			this->timer1->Tick += gcnew System::EventHandler(this, &Form1::timer1_Tick_1);
			// 
			// Form1
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(234, 233);
			this->Controls->Add(this->listBox1);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->comboBox1);
			this->Name = L"Form1";
			this->Text = L"Form1 - MOGI template";
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &Form1::Form1_FormClosing);
			this->Load += gcnew System::EventHandler(this, &Form1::Form1_Load);
			this->ResumeLayout(false);

		}
#pragma endregion
	
		String^ kartya;
		System::IO::StreamWriter^ sw = gcnew System::IO::StreamWriter("cardlog.txt");
		bool n = 0;
		

	private: System::Void Form1_Load(System::Object^ sender, System::EventArgs^ e) {
		Text = "uMOGI";
		button1->Text = "Connect";

		timer1->Interval = 500;	//ms
		timer1->Enabled = true;
		timer1->Start();

		array<String^>^portok = System::IO::Ports::SerialPort::GetPortNames();

		comboBox1->Items->AddRange(portok);

		//System::IO::StreamWriter^ sw = gcnew System::IO::StreamWriter("cardlog.txt");
		sw->WriteLine("rdm-6300 card logs");

		for (int i = 0; i < 18; i++)
		{
			sw->Write("-");
		}
		sw->Write("\n");
		//sw->Close();
		
	}
	private: System::Void button1_Click(System::Object^ sender, System::EventArgs^ e) {

		try {
			serialPort1->PortName = comboBox1->Text;
			serialPort1->BaudRate = 115200;

			serialPort1->Open();
			serialPort1->ReadExisting();
			serialPort1->Write("R");
		}
		catch (Exception^ex){}
	}

private: System::Void serialPort1_DataReceived(System::Object^ sender, System::IO::Ports::SerialDataReceivedEventArgs^ e) {
	kartya = serialPort1->ReadLine();
	System::DateTime now = System::DateTime::Now;
	String^ ido = String::Format("{0:yyyy:MM:dd HH:mm:ss}", now);
	kartya = kartya->Replace(" ", "");
	sw->Write(ido);
	sw->Write("\t");
	sw->Write(kartya);
	sw->Write("\n");
	n = true;
	
}

private: System::Void timer1_Tick_1(System::Object^ sender, System::EventArgs^ e) {
	if (n) {
		listBox1->Items->Add(kartya);
		n = false;
	}
}
private: System::Void Form1_FormClosing(System::Object^ sender, System::Windows::Forms::FormClosingEventArgs^ e) {
	sw->Close();
	if (serialPort1->IsOpen)
		serialPort1->Close();
}
};
}
