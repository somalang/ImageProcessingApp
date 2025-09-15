using System.Windows;

namespace ImageProcessing.Views
{
    public partial class ParameterInputDialog : Window
    {
        public string InputValue { get; private set; }

        public ParameterInputDialog(string title, string prompt, string defaultValue = "")
        {
            InitializeComponent();
            DataContext = this;

            Title = title;
            Prompt = prompt;
            InputTextBox.Text = defaultValue;
        }

        public string Prompt { get; set; }

        private void OkButton_Click(object sender, RoutedEventArgs e)
        {
            InputValue = InputTextBox.Text;
            DialogResult = true;
        }
    }
}
