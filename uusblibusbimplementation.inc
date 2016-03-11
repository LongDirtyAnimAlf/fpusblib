
{ TUSBControllerAsync }

procedure TUSBControllerAsync.DoCheck;
begin
  if (usb_find_busses() > 0)
  or (usb_find_devices() > 0) then
    FParent.Enumerate;
end;

constructor TUSBControllerAsync.Create(aParent : TUSBController);
begin
  FParent := aParent;
  inherited Create(False);
end;

procedure TUSBControllerAsync.Execute;
begin
  while not Terminated do
    begin
      Synchronize(@DoCheck);
      sleep(2000);
    end;
end;

{ TUSBController }

procedure TUSBController.RefreshList;
var
  Bus : PUSBBus;
  aHostController: TUSBHostController = nil;
  i: Integer;
  aRootHub: TUSBHub;

  procedure EnumHubDevices(Hub : TUSBHub);
  var
    aDevice,
    aChildren: PUSBDevice;
    a,i : Integer;
    buff : array[0..511] of char;
    DevHandle: PUSBDevHandle;
    Found: Boolean;
  begin
    aDevice := Bus^.devices;
    a := 0;
    for i := 0 to Hub.Count-1 do
      TUSBGenericDevice(Hub.Items[i]).Tag := 0;
    while Assigned(aDevice) do
      begin
        Found := False;
        for i := 0 to Hub.Count-1 do
          if TUSBGenericDevice(Hub.Items[i]).Path = aDevice^.filename then
            begin
              Found := True;
              TUSBGenericDevice(Hub.Items[i]).Tag := 1;
            end;
        if not Found then
          begin
            a := Hub.Add(TUSBDevice.Create(aDevice^.filename,Hub,Self,dsConnected));
            TUSBDevice(Hub.Items[a]).Tag := 1;
            TUSBDevice(Hub.Items[a]).FDeviceID := aDevice^.descriptor.idProduct;
            TUSBDevice(Hub.Items[a]).FVendorID := aDevice^.descriptor.idVendor;
            DevHandle := usb_open(aDevice);
            if Assigned(DevHandle) then
              begin
                usb_get_string_simple(DevHandle, aDevice^.descriptor.iManufacturer, buff, 512);
                TUSBDevice(Hub.Items[a]).FVendor := buff;
                usb_get_string_simple(DevHandle, aDevice^.descriptor.iProduct, buff, 512);
                TUSBDevice(Hub.Items[a]).FDeviceDescription := buff;
                usb_close(DevHandle);
              end;
            if Assigned(OnUSBArrival) then
              OnUSBArrival(TUSBGenericDevice(Hub.Items[a]));
          end;
        inc(a);
        aDevice := aDevice^.next;
      end;
    i := 0;
    while i < Hub.Count do
      begin
        if TUSBGenericDevice(Hub.Items[i]).Tag = 0 then
          begin
            TUSBGenericDevice(Hub.Items[i]).Free;
            Hub.Delete(i);
          end
        else inc(i);
      end;
{ libusb dont support topologic enumeration
    aChildren := @aDevice^.children;
    while Hub.Count < aDevice^.num_children do
      Hub.Add(nil);
    for a := 0 to aDevice^.num_children-1 do
      begin
        if not Assigned(aChildren) then break;
        Showmessage(Hub.Path+'/'+aChildren^.filename);
        if aChildren^.num_children = 0 then
          begin
            Hub.Items[a] := TUSBDevice.Create(Hub.Path+'/'+aChildren^.filename,Hub,Self,dsConnected);

          end
        else
          begin
            Hub.Items[a] := TUSBHub.Create(Hub.Path+'/'+aChildren^.filename,Hub,Self,dsConnected);
            EnumHubDevices(TUSBHub(Hub.Items[a]));
          end;
        aChildren := aChildren+sizeof(aChildren);
      end;
}
  end;

begin
  usb_find_busses();
  usb_find_devices();
  bus := usb_get_busses;
  while Assigned(Bus) do
    begin
      aHostController := nil;
      for i := 0 to FBusList.Count-1 do
        if TUSBHostController(FBusList[i]).Path = Bus^.dirname then
          begin
            TUSBHostController(FBusList[i]).Tag := 1;
            aHostController := TUSBHostController(FBusList[i]);
          end;
      if not Assigned(aHostController) then
        begin
          aHostController := TUSBHostController.Create(Bus^.dirname);
          aHostController.Tag := 1;
          FBusList.Add(aHostController);
        end;
      if Assigned(aHostController) then
        begin
          if (aHostController.Count > 0) then
            aRootHub := TUSBHub(aHostController.Devices[0])
          else if Bus^.root_dev <> nil then
            begin
              aRootHub := TUSBHub.Create(Bus^.dirname+'/'+Bus^.root_dev^.filename,nil,Self,dsConnected);
              aHostController.Add(aRootHub);
            end;
          EnumHubDevices(aRootHub);
        end;
      Bus := Bus^.next;
    end;
end;

function TUSBController.GetUSBDeviceClass(VendorID, DeviceID: word
  ): TUSBDeviceClass;
begin

end;

constructor TUSBController.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  FBusList := TList.Create;
  FDeviceList := TList.Create;
  usb_init();
  Async := TUSBControllerAsync.Create(Self);
end;

destructor TUSBController.Destroy;
begin
  Async.Free;
  FBusList.Free;
  FDeviceList.Free;
  inherited Destroy;
end;


{ TUSBDevice }

function TUSBDevice.OpenDevice: Boolean;
begin

end;

procedure TUSBDevice.CloseDevice;
begin

end;

constructor TUSBDevice.Create(aDeviceHandle: string;
  aParent: TUSBGenericDevice; aController: TUSBController;
  aStatus: TUSBDeviceStatus);
begin
  inherited Create(aDeviceHandle,aParent,aController,aStatus);
end;

destructor TUSBDevice.Destroy;
begin
  if Assigned(Controller.OnUSBRemove) then
    Controller.OnUSBRemove(Self);
  inherited Destroy;
end;

