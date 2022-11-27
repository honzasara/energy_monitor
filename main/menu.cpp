#include "menu.h"
#include "SSD1306.h"
#include "Font5x8.h"

uint8_t menu_index = 0;
uint8_t display_redraw = true;
uint8_t menu_counter[MAX_MENUS];
uint8_t menu_params_1[MAX_MENUS];
uint8_t menu_params_2[MAX_MENUS];


uint8_t display_menu(uint8_t button_up, uint8_t button_down, uint8_t button_enter, uint8_t redraw)
{
  uint8_t new_redraw = false;
  char str1[20];
  if (redraw == true)
  {
    switch (menu_index)
    {
      case INDEX_MENU_SET_IP:
	strcpy(str1, "IP");
	new_redraw = display_menu_set_ip(button_up, button_down, button_enter, device.myIP, str1);
	break;
      case INDEX_MENU_SET_MASK:
	strcpy(str1, "MASK");
        new_redraw = display_menu_set_ip(button_up, button_down, button_enter, device.myMASK, str1);
        break;
      case INDEX_MENU_SET_GW:
	strcpy(str1, "GW");
        new_redraw = display_menu_set_ip(button_up, button_down, button_enter, device.myGW, str1);
        break;
      case INDEX_MENU_SET_DNS:
	strcpy(str1, "DNS");
        new_redraw = display_menu_set_ip(button_up, button_down, button_enter, device.myDNS, str1);
        break;
      case INDEX_MENU_SET_MQTT:
	strcpy(str1, "MQTT IP");
        new_redraw = display_menu_set_ip(button_up, button_down, button_enter, device.mqtt_server, str1);
        break;
      case INDEX_MENU_SET_MQTT_USER:
	strcpy(str1, "MQTT user");
	new_redraw = display_menu_set_name(button_up, button_down, button_enter, device.mqtt_user, str1, 20);
	break;
      case INDEX_MENU_SET_MQTT_PASS:
	strcpy(str1, "MQTT key");
        new_redraw = display_menu_set_name(button_up, button_down, button_enter, device.mqtt_key, str1, 20);
        break;
      case INDEX_MENU_SET_BOOT_URI:
	strcpy(str1, "boot uri");
        new_redraw = display_menu_set_name(button_up, button_down, button_enter, device.bootloader_uri, str1, 40);
        break;
      case INDEX_MENU_SET_DEVICE_NAME:
	strcpy(str1, "nazev");
        new_redraw = display_menu_set_name(button_up, button_down, button_enter, device.nazev, str1, 10);
        break;
      case INDEX_MENU_SET_WIFI_ESSID:
	strcpy(str1, "wifi essid");
        new_redraw = display_menu_set_name(button_up, button_down, button_enter, device.wifi_essid, str1, 20);
        break;
      case INDEX_MENU_SET_WIFI_PASS:
	strcpy(str1, "wifi pass");
        new_redraw = display_menu_set_name(button_up, button_down, button_enter, device.wifi_key, str1, 20);
        break;
      case INDEX_MENU_SET_DHCP:
	//// neni potreba resit menu
	break;
      case INDEX_MENU_MAIN:
        new_redraw = display_menu_main(button_up, button_down, button_enter);
        break;
      case INDEX_MENU_DEFAULT:
        new_redraw = display_menu_default(button_up, button_down, button_enter);
        break;
      default:
        break;
    }
  }
  return new_redraw;
}



uint8_t display_menu_set_name(uint8_t button_up, uint8_t button_down, uint8_t button_enter, char *name, char *label, uint8_t max_len)
{
  uint8_t klik = false;
  uint8_t skip = false;
  char str1[32] = {0};
  char str2[32] = {0};
  uint8_t pos_index = 0;
  uint8_t pos_cursor = 0;
  uint8_t pos_select_cursor = 0;
  char z;
  char map[65];
  uint8_t t=0;
  for (uint8_t idx='a'; idx<='z'; idx++)
    {
    map[t] = idx;
    t++;
    }
  for (uint8_t idx='A'; idx<='Z'; idx++)
    {
    map[t] = idx;
    t++;
    }
  for (uint8_t idx='0'; idx<='9'; idx++)
    {
    map[t] = idx;
    t++;
    }
  map[t] = ':';
  t++;
  map[t] = '/';
  t++;
  map[t] = '.';

  pos_index = 0;
  pos_cursor = 0;
  if (menu_params_1[menu_index] > 0)
  {
     pos_index = menu_params_1[menu_index] -1;
     if (pos_index >= 10)
        pos_cursor = pos_index - 9;
  }
  if (pos_cursor + 10 >= strlen(name) && strlen(name) > 10)
     pos_cursor = strlen(name) - 10;

  if (pos_index < 10)
     pos_select_cursor = pos_index;
  if (pos_index >= 10)
     pos_select_cursor = 9;


  if (menu_params_2[menu_index] == MENU_SET_NETWORK_PARAMS_SHOW)
  {
     if (button_enter == BUTTON_PRESSED)
     {
     klik = true;
     /// tlacitko zpet
     if (menu_params_1[menu_index] == 0)
     {
        menu_index = INDEX_MENU_MAIN;
        skip = true;
     }
     /// klik na pismenko, ktere potrebuji editovat
     if (menu_params_1[menu_index] > 0 && menu_params_1[menu_index] < strlen(name)+1)
     {
        menu_params_2[menu_index] = MENU_SET_NETWORK_PARAMS_SETTING;
        skip = true;
        goto display_menu_set_name_jump_1;
     }
     /// tlacitko odmaz znak
     if (menu_params_1[menu_index] == (strlen(name)+1))
        if (strlen(name) > 0)
        {
           name[strlen(name)-1] = '\0';
           goto display_menu_set_name_jump_1;
        }
     /// tlacitko pridej znak
     if (menu_params_1[menu_index] == (strlen(name)+2))
        if (strlen(name) < (max_len - 1))
        {
           strcat(name, "a");
           goto display_menu_set_name_jump_1;
        }
     }
     /// v menu dalsi
     if (button_up == BUTTON_PRESSED)
     {
        klik = true;
        menu_params_1[menu_index]++;
        if (menu_params_1[menu_index] > strlen(name)+2)
           menu_params_1[menu_index] = 0;
     }
     /// v menu zpet
     if (button_down == BUTTON_PRESSED)
     {
        klik = true;
        if (menu_params_1[menu_index] == 0)
           menu_params_1[menu_index] = strlen(name)+1+2;
        menu_params_1[menu_index]--;
     }
  }

  if (menu_params_2[menu_index] == MENU_SET_NETWORK_PARAMS_SETTING && menu_params_1[menu_index] > 0 && pos_index < strlen(name))
     {
     /// jsem v editacnim modu, vyskocim ven
     if (button_enter == BUTTON_PRESSED)
        {
        klik = true;
	menu_params_2[menu_index] = MENU_SET_NETWORK_PARAMS_SHOW;
	}
     /// inkremnetuji pismenka
     if (button_up == BUTTON_PRESSED)
	{
        z = name[pos_index];
	for (uint8_t idx_t = 0; idx_t < 65; idx_t++)
	   {
	   t = idx_t;
	   if (map[t] == z)
	      {
	      if (t == 64) t = 0;
	      z = map[t+1];
	      break;
	      }
	   }
        name[pos_index]=z;
	}
     /// dekrementuji pismenka
     if (button_down == BUTTON_PRESSED)
	{
	z = name[pos_index];
	for (uint8_t idx_t = 0; idx_t < 65; idx_t++)
	   {
	   t = idx_t;
           if (map[t] == z)
              {
	      if (t == 0) t = 63;
              z = map[t-1];
              break;
              }
	   }
        name[pos_index]=z;
	}
     }

display_menu_set_name_jump_1:
  if (skip == false)
  {
  GLCD_Clear();
  GLCD_GotoXY(0, 0);
  strcpy(str1, "Nastaveni:");
  strcat(str1, label);
  GLCD_PrintString(str1);

  GLCD_GotoXY(2, 16);
  strcpy(str1, "<-");
  GLCD_PrintString(str1);
  if (menu_params_1[menu_index] == 0)
  GLCD_DrawRectangle(0, 14, 2 + GLCD_GetWidthString(str1), 24, GLCD_Black);

  if (strlen(name) > 10 )
     strncpy(str2, name + pos_cursor, 10);
  else
     strcpy(str2, name);

  strcat(str2, "");
  GLCD_GotoXY(20, 9);
  GLCD_PrintString(str2);
 
  if (menu_params_2[menu_index] == MENU_SET_NETWORK_PARAMS_SHOW)
  {
     if (menu_params_1[menu_index] > 0 && pos_index < strlen(name))
     { 
       strcpy(str1, "^");
       GLCD_GotoXY(20 + (pos_select_cursor * 6), 22);
       GLCD_PrintString(str1);
     }
  }
  if (menu_params_2[menu_index] == MENU_SET_NETWORK_PARAMS_SETTING)
  {
     GLCD_DrawRectangle(19 + (pos_select_cursor * 6), 9, 20 + (pos_select_cursor * 6) + 5, 17, GLCD_Black);
  }

  GLCD_GotoXY(90, 16);
  strcpy(str1, "<<");
  GLCD_PrintString(str1);
  if (menu_params_1[menu_index] == (strlen(name)+1))
     GLCD_DrawRectangle(88, 14, 88 + GLCD_GetWidthString(str1), 24, GLCD_Black);  

  GLCD_GotoXY(110, 16);
  strcpy(str1, ">>");
  GLCD_PrintString(str1); 
  if (menu_params_1[menu_index] == (strlen(name)+2))
     GLCD_DrawRectangle(108, 14, 108 + GLCD_GetWidthString(str1), 24, GLCD_Black);

  GLCD_Render();
  }
  return klik;
}


uint8_t display_menu_set_ip(uint8_t button_up, uint8_t button_down, uint8_t button_enter, uint8_t *ip, char *label)
{
  char str1[5];
  char str2[32];
  uint8_t klik = false;
  uint8_t skip = false;
  /// mam menu v ukazovacim rezimu
  if (menu_params_2[menu_index] == MENU_SET_NETWORK_PARAMS_SHOW)
     {
     if (button_enter == BUTTON_PRESSED)
         {
         klik = true;
         /// tlacitko zpet
         if (menu_params_1[menu_index] == 0)
           {
           menu_index = INDEX_MENU_MAIN;
           skip = true;
           }
         /// u vybraneho bytu adresy, si skocim do noveho menu
         if (menu_params_1[menu_index] != 0)
           {
           button_enter = BUTTON_NONE;
	   menu_params_2[menu_index] = MENU_SET_NETWORK_PARAMS_SETTING;
           skip = true;
           }
         }
       if (button_up == BUTTON_PRESSED)
          {
          klik = true;
          menu_params_1[menu_index]++;
          if (menu_params_1[menu_index] > 4)
                menu_params_1[menu_index] = 0;
          }
       if (button_down == BUTTON_PRESSED)
         {
         klik = true;
         if (menu_params_1[menu_index] == 0)
               menu_params_1[menu_index] = 5;
         menu_params_1[menu_index]--;
         }
    }
  /// mam menu v editovacim rezimu
  if (menu_params_2[menu_index] == MENU_SET_NETWORK_PARAMS_SETTING)
     {
     if (button_up == BUTTON_PRESSED)
          {
          klik = true;
	  ip[menu_params_1[menu_index]-1]++;
	  }
     if (button_down == BUTTON_PRESSED)
          {
          klik = true;
          ip[menu_params_1[menu_index]-1]--;
          }
     if (button_enter == BUTTON_PRESSED)
         {
         klik = true;
	 menu_params_2[menu_index] = MENU_SET_NETWORK_PARAMS_SHOW;
	 }
     }
  if (skip == false)
  {
  GLCD_Clear();
  GLCD_GotoXY(0, 0);
  strcpy(str2, "Nastaveni:");
  strcat(str2, label);
  GLCD_PrintString(str2);
  GLCD_GotoXY(2, 16); 
  strcpy(str1, "<-");
  GLCD_PrintString(str1);
  if (menu_params_1[menu_index] == 0)
  GLCD_DrawRectangle(0, 14, 2 + GLCD_GetWidthString(str1), 24, GLCD_Black);
  GLCD_GotoXY(18, 16);
  itoa(ip[0], str1, 10);
  strcat(str1, "");
  GLCD_PrintString(str1);
  if (menu_params_1[menu_index] == 1)
  GLCD_DrawRectangle(16, 14, 18 + GLCD_GetWidthString(str1), 24, GLCD_Black);
  GLCD_GotoXY(48, 16);
  itoa(ip[1], str1, 10);
  strcat(str1, "");
  GLCD_PrintString(str1);
  if (menu_params_1[menu_index] == 2)
  GLCD_DrawRectangle(46, 14, 48 + GLCD_GetWidthString(str1), 24, GLCD_Black);
  GLCD_GotoXY(78, 16);
  itoa(ip[2], str1, 10);
  strcat(str1, "");
  GLCD_PrintString(str1);
  if (menu_params_1[menu_index] == 3)
  GLCD_DrawRectangle(76, 14, 78 + GLCD_GetWidthString(str1), 24, GLCD_Black);
  GLCD_GotoXY(108, 16);
  itoa(ip[3], str1, 10);
  strcat(str1, "");
  GLCD_PrintString(str1);
  if (menu_params_1[menu_index] == 4)
  GLCD_DrawRectangle(106, 14, 108 + GLCD_GetWidthString(str1), 24, GLCD_Black);
  GLCD_Render();
  }
  return klik;
}





uint8_t display_menu_main(uint8_t button_up, uint8_t button_down, uint8_t button_enter)
{
  uint8_t klik = false;
  uint8_t skip = false;
  if (button_up == BUTTON_PRESSED)
    {
    klik = true;
    menu_params_1[menu_index]++;
    if (menu_params_1[menu_index] > 16)
	  menu_params_1[menu_index] = 0;
    }

  if (button_down == BUTTON_PRESSED)
    {
    klik = true;
    if (menu_params_1[menu_index] == 0)
	  menu_params_1[menu_index] = 17;
    menu_params_1[menu_index]--;
    }

  if (button_enter == BUTTON_PRESSED)
    {
    /// tlacitko povoleni interface
    /// TODO
    /// tlacitko dhcp enable/disable, neni dalsi menu
    if (menu_params_1[menu_index] == 16)
      {
      if (device.dhcp == DHCP_ENABLE)
	 {
	 device.dhcp = DHCP_DISABLE;
         goto display_menu_main_jump_1;
         }
      if (device.dhcp == DHCP_DISABLE)
	 {
	 device.dhcp = DHCP_ENABLE;
         goto display_menu_main_jump_1;
	 }
      }

    /// tlacitko o wifi essid
    if (menu_params_1[menu_index] == 15)
      {
      menu_index = INDEX_MENU_SET_WIFI_PASS;
      menu_params_1[menu_index] = 0;
      menu_params_2[menu_index] = 0;
      skip = true;
      goto display_menu_main_jump_1;
      }
    /// tlacitko o wifi essid
    if (menu_params_1[menu_index] == 14)
      {
      menu_index = INDEX_MENU_SET_WIFI_ESSID;
      menu_params_1[menu_index] = 0;
      menu_params_2[menu_index] = 0;
      skip = true;
      goto display_menu_main_jump_1;
      }
    /// tlacitko o zarizeni
    if (menu_params_1[menu_index] == 13)
      {
      menu_index = INDEX_MENU_ABOUT_DEVICE;
      menu_params_1[menu_index] = 0;
      menu_params_2[menu_index] = 0;
      skip = true;
      goto display_menu_main_jump_1;
      }
    /// tlacitko zpet
    if (menu_params_1[menu_index] == 12)
      {
      menu_index = INDEX_MENU_DEFAULT;
      button_enter = BUTTON_NONE;
      skip = true;
      goto display_menu_main_jump_1;
      }
    /// tlacitko nastaveni nazev
    if (menu_params_1[menu_index] == 11)
      {
      menu_index = INDEX_MENU_SET_DEVICE_NAME;
      menu_params_1[menu_index] = 0;
      menu_params_2[menu_index] = 0;
      skip = true;
      goto display_menu_main_jump_1;
      }
    klik = true;
    /// tlacitko default
    if (menu_params_1[menu_index] == 10)
      {
      menu_index = INDEX_MENU_DEFAULT;
      button_enter = BUTTON_NONE;
      EEPROM.write(set_default_values, 255);
      esp_restart();
      skip = true;
      goto display_menu_main_jump_1;
      }
    /// tlacitko ulozit a restartovat
    if (menu_params_1[menu_index] == 9)
      {
      save_setup_network();
      menu_index = INDEX_MENU_DEFAULT;
      button_enter = BUTTON_NONE;
      esp_restart();
      skip = true;
      goto display_menu_main_jump_1; 
      }
    /// tlacitko nastaveni IP
    if (menu_params_1[menu_index] == 1)
      {
      menu_index = INDEX_MENU_SET_IP;
      menu_params_1[menu_index] = 0;
      menu_params_2[menu_index] = 0;
      skip = true;
      goto display_menu_main_jump_1;
      }
    /// tlacitko nastaveni MASKY
    if (menu_params_1[menu_index] == 2)
      {
      menu_index = INDEX_MENU_SET_MASK;
      menu_params_1[menu_index] = 0;
      menu_params_2[menu_index] = 0;
      skip = true;
      goto display_menu_main_jump_1;
      }
    /// tlacitko nastaveni GW
    if (menu_params_1[menu_index] == 3)
      {
      menu_index = INDEX_MENU_SET_GW;
      menu_params_1[menu_index] = 0;
      menu_params_2[menu_index] = 0;
      skip = true;
      goto display_menu_main_jump_1;
      }
    /// tlacitko nastaveni DNS
    if (menu_params_1[menu_index] == 4)
      {
      menu_index = INDEX_MENU_SET_DNS;
      menu_params_1[menu_index] = 0;
      menu_params_2[menu_index] = 0;
      skip = true;
      goto display_menu_main_jump_1;
      }
    /// tlacitko nastaveni MQTT
    if (menu_params_1[menu_index] == 5)
      {
      menu_index = INDEX_MENU_SET_MQTT;
      menu_params_1[menu_index] = 0;
      menu_params_2[menu_index] = 0;
      skip = true;
      goto display_menu_main_jump_1;
      }
    /// tlacitko nastaveni MQTT user
    if (menu_params_1[menu_index] == 6)
      {
      menu_index = INDEX_MENU_SET_MQTT_USER;
      menu_params_1[menu_index] = 0;
      menu_params_2[menu_index] = 0;
      skip = true;
      goto display_menu_main_jump_1;
      }
    /// tlacitko nastaveni MQTT key
    if (menu_params_1[menu_index] == 7)
      {
      menu_index = INDEX_MENU_SET_MQTT_PASS;
      menu_params_1[menu_index] = 0;
      menu_params_2[menu_index] = 0;
      skip = true;
      goto display_menu_main_jump_1;
      }
    /// tlacitko nastaveni boot uri
    if (menu_params_1[menu_index] == 8)
      {
      menu_index = INDEX_MENU_SET_BOOT_URI;
      menu_params_1[menu_index] = 0;
      menu_params_2[menu_index] = 0;
      skip = true;
      goto display_menu_main_jump_1;
      }
    }
display_menu_main_jump_1:
  if (skip == false)
    {
    GLCD_Clear();
    GLCD_GotoXY(0, 0);
    GLCD_PrintString("Wattmetr hlavni menu");
    if (menu_params_1[menu_index] == 0)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Spotreba");
      }
    if (menu_params_1[menu_index] == 1)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Nastaveni IP");
      }
    if (menu_params_1[menu_index] == 2)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Nastaveni masky");
      }
    if (menu_params_1[menu_index] == 3)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Nastaveni GW");
      }
    if (menu_params_1[menu_index] == 4)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Nastaveni DNS");
      }
    if (menu_params_1[menu_index] == 5)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Nastaveni MQTT IP");
      }
    if (menu_params_1[menu_index] == 6)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Nastaveni MQTT User");
      }
    if (menu_params_1[menu_index] == 7)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Nastaveni MQTT pass");
      }
    if (menu_params_1[menu_index] == 8)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Nastaveni boot URI");
      }
    if (menu_params_1[menu_index] == 9)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Ulozit a restartovat");
      }
    if (menu_params_1[menu_index] == 10)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Vychozi nastaveni");
      }
    if (menu_params_1[menu_index] == 11)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Nazev zarizeni");
      }
    if (menu_params_1[menu_index] == 12)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("Zpet");
      }
    if (menu_params_1[menu_index] == 13)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("O zarizeni");
      }
    if (menu_params_1[menu_index] == 14)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("WIFI essid");
      }
    if (menu_params_1[menu_index] == 15)
      {
      GLCD_GotoXY(0, 16);
      GLCD_PrintString("WIFI pass");
      }
    if (menu_params_1[menu_index] == 16)
      {
      GLCD_GotoXY(0, 16);
      if (device.dhcp == DHCP_ENABLE)
          GLCD_PrintString("DHCP enable");
      if (device.dhcp == DHCP_DISABLE)
	 GLCD_PrintString("DHCP disable");
      }

    GLCD_Render();
    }

  return klik;
}





uint8_t display_menu_default(uint8_t button_up, uint8_t button_down, uint8_t button_enter)
{
 uint8_t klik = false;
 uint8_t skip = false;
 struct_energy_t energyx;
 char str1[24] = {0};
 int32_t total_miliamp = 0;
 int32_t total_miliwats = 0;

 if (button_enter == BUTTON_PRESSED)
   {
   menu_index = INDEX_MENU_MAIN;
   klik = true;
   skip = true;
   }

 if (skip == false)
   {
   GLCD_Clear();
   
   GLCD_GotoXY(16, 0);
   if (menu_params_1[menu_index] == 0) GLCD_PrintString("wattmetr v0.1 -");
   if (menu_params_1[menu_index] == 1) GLCD_PrintString("wattmetr v0.1 \\");
   if (menu_params_1[menu_index] == 2) GLCD_PrintString("wattmetr v0.1 |");
   if (menu_params_1[menu_index] == 3) GLCD_PrintString("wattmetr v0.1 /");

   GLCD_GotoXY(0, 23);
   if (mqtt_link == MQTT_EVENT_CONNECTED)
      GLCD_PrintString("MQTT:OK");
   else
     GLCD_PrintString("MQTT:err");

   if ((menu_params_1[menu_index] % 2) == 0)
   {
   GLCD_GotoXY(0, 13);
   if (eth_link == ETHERNET_EVENT_CONNECTED)
     GLCD_PrintString("ETH:OK");
   else
     GLCD_PrintString("ETH:nolink");
   }

   if ((menu_params_1[menu_index] % 2) == 1)
   {
   GLCD_GotoXY(0, 13);
   if (wifi_link == WIFI_EVENT_STA_CONNECTED)
     GLCD_PrintString("WIFI:OK");
   else
     GLCD_PrintString("WIF:nolink");
   }

   if (menu_params_2[menu_index] == 0)
     {
     energy_get_all_parametr(0, &energyx);
     GLCD_GotoXY(60, 23);
     sprintf(str1, "F%d: %dW", energyx.id, (uint16_t)((float)(energyx.current_now * energyx.volt))/1000);
     strcat(str1,"");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "F%d: %.02fA", energyx.id, (float)energyx.current_now/1000);
     strcat(str1,"");
     GLCD_PrintString(str1);
     }
   if (menu_params_2[menu_index] == 1)
     {
     energy_get_all_parametr(0, &energyx);
     GLCD_GotoXY(60, 23);
     sprintf(str1, "F%d: %s", energyx.id, energyx.name);
     strcat(str1,"");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "F%d: %ldWh", energyx.id, energyx.total_watt);
     strcat(str1,"");
     GLCD_PrintString(str1);
     }



   if (menu_params_2[menu_index] == 2)
     {
     energy_get_all_parametr(1, &energyx);
     GLCD_GotoXY(60, 23);
     sprintf(str1, "F%d: %dW", energyx.id, (uint16_t)((float)(energyx.current_now * energyx.volt))/1000);
     strcat(str1,"");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "F%d: %.02fA", energyx.id, (float)energyx.current_now/1000);
     strcat(str1,"");
     GLCD_PrintString(str1);
     }
   if (menu_params_2[menu_index] == 3)
     {
     energy_get_all_parametr(1, &energyx);
     GLCD_GotoXY(60, 23);
     sprintf(str1, "F%d: %s", energyx.id, energyx.name);
     strcat(str1,"");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "F%d: %ldWh", energyx.id, energyx.total_watt);
     strcat(str1,"");
     GLCD_PrintString(str1);
     }



   if (menu_params_2[menu_index] == 4)
     {
     energy_get_all_parametr(2, &energyx);
     GLCD_GotoXY(60, 23);
     sprintf(str1, "F%d: %dW", energyx.id, (uint16_t)((float)(energyx.current_now * energyx.volt))/1000);
     strcat(str1,"");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "F%d: %.02fA", energyx.id, (float)energyx.current_now/1000);
     strcat(str1,"");
     GLCD_PrintString(str1);
     }
   if (menu_params_2[menu_index] == 5)
     {
     energy_get_all_parametr(2, &energyx);
     GLCD_GotoXY(60, 23);
     sprintf(str1, "F%d: %s", energyx.id, energyx.name);
     strcat(str1,"");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "F%d: %ldWh", energyx.id, energyx.total_watt);
     strcat(str1,"");
     GLCD_PrintString(str1);
     }


   if (menu_params_2[menu_index] == 6)
     {
     energy_get_all_parametr(3, &energyx);
     GLCD_GotoXY(60, 23);
     sprintf(str1, "F%d: %dW", energyx.id, (uint16_t)((float)(energyx.current_now * energyx.volt))/1000);
     strcat(str1,"");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "F%d: %.02fA", energyx.id, (float)energyx.current_now/1000);
     strcat(str1,"");
     GLCD_PrintString(str1);
     }
   if (menu_params_2[menu_index] == 7)
     {
     energy_get_all_parametr(3, &energyx);
     GLCD_GotoXY(60, 23);
     sprintf(str1, "F%d: %s", energyx.id, energyx.name);
     strcat(str1,"");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "F%d: %ldWh", energyx.id, energyx.total_watt);
     strcat(str1,"");
     GLCD_PrintString(str1);
     }


   if (menu_params_2[menu_index] == 8)
     {
     energy_get_all_parametr(4, &energyx);
     GLCD_GotoXY(60, 23);
     sprintf(str1, "F%d: %dW", energyx.id, (uint16_t)((float)(energyx.current_now * energyx.volt))/1000);
     strcat(str1,"");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "F%d: %.02fA", energyx.id, (float)energyx.current_now/1000);
     strcat(str1,"");
     GLCD_PrintString(str1);
     }
   if (menu_params_2[menu_index] == 9)
     {
     energy_get_all_parametr(4, &energyx);
     GLCD_GotoXY(60, 23);
     sprintf(str1, "F%d: %s", energyx.id, energyx.name);
     strcat(str1,"");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "F%d: %ldWh", energyx.id, energyx.total_watt);
     strcat(str1,"");
     GLCD_PrintString(str1);
     }

   if (menu_params_2[menu_index] == 10)
     {
     energy_get_all_parametr(5, &energyx);
     GLCD_GotoXY(60, 23);
     sprintf(str1, "F%d: %dW", energyx.id, (uint16_t)((float)(energyx.current_now * energyx.volt))/1000);
     strcat(str1,"");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "F%d: %.02fA", energyx.id, (float)energyx.current_now/1000);
     strcat(str1,"");
     GLCD_PrintString(str1);
     }
   if (menu_params_2[menu_index] == 11)
     {
     energy_get_all_parametr(5, &energyx);
     GLCD_GotoXY(60, 23);
     sprintf(str1, "F%d: %s", energyx.id, energyx.name);
     strcat(str1,"");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "F%d: %ldWh", energyx.id, energyx.total_watt);
     strcat(str1,"");
     GLCD_PrintString(str1);
     }

   if (menu_params_2[menu_index] == 12)
     {
     total_miliamp = 0;
     total_miliwats = 0;
     for (uint8_t idx = 0; idx < MAX_ADC_INPUT; idx++)
	 {
	 energy_get_all_parametr(idx, &energyx);
	 if (energyx.group == 1)
            {
            if ((energyx & (1 << flag_direction_current)) == 0)
	        {
	        total_miliwats = total_miliwats + (energyx.current_now * energyx.volt);
	        total_miliamp = total_miliamp + energyx.current_now;
		}
	    else
	        {
	        total_miliwats = total_miliwats - (energyx.current_now * energyx.volt);
                total_miliamp = total_miliamp - energyx.current_now;	
		}
	    }
	 }
     GLCD_GotoXY(60, 23);
     sprintf(str1, "G1 %dW", (int16_t)(float)total_miliwats/1000);
     strcat(str1, "");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "G1 %.02fA", (float)total_miliamp/1000);
     strcat(str1, "");
     GLCD_PrintString(str1);
     }

   if (menu_params_2[menu_index] == 13)
     {
     total_miliamp = 0;
     total_miliwats = 0;
     for (uint8_t idx = 0; idx < MAX_ADC_INPUT; idx++)
         {
         energy_get_all_parametr(idx, &energyx);
         if (energyx.group == 2)
            {
	    if ((energyx & (1 << flag_direction_current)) == 0)
		{
                total_miliwats = total_miliwats + (energyx.current_now * energyx.volt);
                total_miliamp = total_miliamp + energyx.current_now;
	        }
	    else
		{
		total_miliwats = total_miliwats - (energyx.current_now * energyx.volt);
                total_miliamp = total_miliamp - energyx.current_now;
		}
            }
         }
     GLCD_GotoXY(60, 23);
     sprintf(str1, "G2 %dW", (uint16_t)(float)total_miliwats/1000);
     strcat(str1, "");
     GLCD_PrintString(str1);
     GLCD_GotoXY(60, 10);
     sprintf(str1, "G2 %.02fA", (float)total_miliamp/1000);
     strcat(str1, "");
     GLCD_PrintString(str1);
     }

   menu_params_1[menu_index]++;
   if (menu_params_1[menu_index] == 4)  menu_params_1[menu_index] = 0;
   menu_params_2[menu_index]++;
   if (menu_params_2[menu_index] == 14)  menu_params_2[menu_index] = 0;

   GLCD_Render();
   }

 return klik;
}


