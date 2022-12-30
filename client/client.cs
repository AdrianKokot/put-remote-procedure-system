using System;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;

namespace TcpWindowsClient
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        public void Main() {
            // Establish a connection to the server
            TcpClient client = new TcpClient();
            string configFile = @"client/config";
            string[] server_data = File.ReadAllLines(configFile);
            client.Connect(server_data[0], server_data[1]);
            NetworkStream stream = client.GetStream();
        }


        private void ExecuteScriptButton_Click(object sender, EventArgs e)
        {
            // Send a message to the server indicating the script to execute and any necessary parameters
            string message = "EXECUTE " + scriptNameTextBox.Text + " " + scriptParametersTextBox.Text;
            byte[] data = Encoding.ASCII.GetBytes(message);
            stream.Write(data, 0, data.Length);

            // Receive the results of the script execution from the server
            byte[] response = new byte[4096];
            int bytesReceived = stream.Read(response, 0, response.Length);
            string responseMessage = Encoding.ASCII.GetString(response, 0, bytesReceived);

            // Display the results to the user
            scriptOutputTextBox.Text = responseMessage;
        }
    }
}
