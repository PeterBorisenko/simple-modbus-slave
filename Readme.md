## How to use this driver:
### Running tests:
* Install ruby. You can obtain ruby from https://www.ruby-lang.org
* Install ceedling. From your command prompt run:
    ```sh 
    gem install ceedling
    ```
* Navigate to ***this_library_sources/Test*** directory
* Press Shift+RMB and select "Open command prompt" from context menu
* To launch the test sequence type:
    ```sh
    rake test:all
    ```
* All tests should be passed
### Adding library to your project
* Add ***this_library_sources/Include*** directory to include path in your IDE or makefile
* Add files from ***this_library_sources/Source*** directory to sources path or just copy it into your Source folder
* Use ***modbus_user_data.template.h*** and ***proto_bindings.template.h*** to implement your own ***modbus_user_data.h*** and ***proto_bindings.h***, placed in you project Include folder.
### Binding to low level drivers:
* You should implement two drivers with appropriate API. The first is Receiver/Transmitter (usually UART)
The second is hardware timer which will run in one-shot mode. Software/Virtual timers can also be used.
* See the ***timer_driver.template.h*** and ***uart_driver.template.h*** to understand what API do you need.
* Write appropriate functions to your proto_bindings.h file.
### Implementing in the Application:
* Define an entity of proto_t and ***modbus_user_data_t***.
* Define and initialize an entity of ***proto_init_t***.
* Assign actual data addresses to the pointers, defined in ***modbus_user_data_t***.
    	Keep in mind that modbus registers are 16-bit wide.
* Set WE field on those registers that should be write accessible.
* Initialize protocol state using ***protoInit()***.
* Place ***protoBackgroundProcess()*** to your main loop or call it periodically
* See an example in example folder.

`warning` Only 2 Modbus functions are currently implemented: READ HOLDING REGISTERS and WRITE SINGLE REGISTER,
which is enough in most situations.<br>
`warning` Only static address are used in this version (Reading and assigning device address at initialization stage)
