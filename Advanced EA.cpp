//+--------------------------------------------------------------------------------+
//|                                                            SilentExpertAdvisor |
//|                                                    Copyright 2020, CompanyName |
//|                                                     http://www.companyname.net |
//+--------------------------------------------------------------------------------+
#property strict

//GLOBAL SETTINGS
input group                                           "Global Settings";
input string               selectSymbol               = "ALL";                                  //Select CURRENT, ALL or self selected Symbols
input ENUM_TIMEFRAMES      selectTimeFrame            = PERIOD_H12;                             //Select Timeframe
input int                  selectShift                = 0;                                      //Select Shift 

//MONEY MANAGMENT
input group                                           "Money Managment";
input string               selectLotSize              = "Pico Lot";                             //Select Lot Size (Standard, Mini, Micro, Nano, Pico, -Lot)
input double               normalTakeProfitModifier   = 2;                                      //Take Profit Multiplier (Normal Position)
input double               firstTradeModifier         = 3;                                      //First Trade Modifier (TakeProfit - Open Price / TradeModifier)
input double               secondTradeModifier        = 2;                                      //Second Trade Modifier (TakeProfit - Open Price / TradeModifier)

//RISK MANAGMENT
input group                                           "Risk Managment";
input double               riskFactor                 = 0.02;                                   //Select Risk Factor (0.02 --> 2%)
input double               stopLossFactor             = 1.5;                                    //Stop Loss Factor
input double               takeProfitFactor           = 1.0;                                    //Take Profit Factor

//INDICATOR SETTINGS
input group                                           "General Indicator Settings";
input int                  numValuesNeeded            = 3;                                      //Select how many Values IndicatorHandles save

input group                                           "William %R Settings";                                   
input int                  selectWPRPeriod            = 14;                                     //WPR Period

input group                                           "Force Index Settings";
input int                  selectFORCEPeriod          = 13;                                     //FORCE Period
input ENUM_MA_METHOD       selectFORCEMode            = MODE_SMA;                               //FORCE Mode
input ENUM_APPLIED_VOLUME  selectFORCEAppliedVolume   = VOLUME_TICK;                            //FORCE Volume

input group                                           "Moving Average Settings";
input int                  selectMAPeriod             = 7;                                      //MA Period
input int                  selectMAShift              = 0;                                      //MA Shift
input ENUM_MA_METHOD       selectMAMode               = MODE_SMA;                               //MA Mode
input ENUM_APPLIED_PRICE   selectMAAppliedPrice       = PRICE_CLOSE;                            //MA Applied Price

input group                                           "William %R Moving Average Settings";
input int                  selectWPRMAPeriod          = 10;                                     //WPRMA Period
input int                  selectWPRMAShift           = 0;                                      //WPRMA Shift
input ENUM_MA_METHOD       selectWPRMAMode            = MODE_SMA;                               //WPRMA Mode

input group                                           "Average True Range Settings";
input int                  selectATRPeriod            = 14;                                     //ATR Period

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
//INDICATOR HANDLES
int handleWPR[], handleFORCE[], handleMA[], handleATR[], handleWPRMA[];

//ARRAYS
//string allSymbols = "EURUSD";
//string allSymbols = "EURUSD|Apple|Tesla|AmericanAirlines|Lufthansa|EURJPY";
string allSymbols = "EURAUD|EURCHF|EURGBP|EURJPY|EURTRY|EURUSD|USDJPY|USDCAD|USDSGD|AUDUSD|AUDNZD|AUDJPY|AUDCAD|GBPUSD|GBPAUD";
string expertAdvisorName = "SYSTEMMESSAGE//EYE";
string symbolArray[];
int numberOfTradeableSymbols, ticksReceivedCount;
ulong openTradeOrderTicket[];
int signalWPRLong, signalWPRMALong, signalMALong, signalVolumeLong, signalWPRShort, signalWPRMAShort, signalMAShort, signalVolumeShort;
int signalWPRCloseLong, signalWPRCloseShort;
int lotSize = 1;
double partialCloseFactor = 1;
double positionSize = 1;

 


//+--------------------------------------------------------------------------------+
//|                                                            SilentExpertAdvisor |
//|                                                    Copyright 2020, CompanyName |
//|                                                     http://www.companyname.net |
//+--------------------------------------------------------------------------------+

//TO DO LIST
// - Calculate Trading Volume for each Symbol
// - Trade multiple Symbols at a time
// - Close Orders on command

#property strict
#include <Trade\Trade.mqh>
#include <SilentInclude\Global Inputs.mqh>
#include <SilentInclude\Global Functions.mqh>
CTrade trade;

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
int OnInit()
  {
   //Calibrating Functions
   GetInitializeInformation();
   ResizeCoreArrays();
   ResizeIndicatorHandleArrays();

   //Setting up Order Tickets (Out of use)
   for(int symbolLoop = 0; symbolLoop < numberOfTradeableSymbols; symbolLoop++)
     {
         openTradeOrderTicket[symbolLoop] = 0;
     } 
   
   //Setting up Indicator Handles for each Symbol
   if(!SetUpIndicatorHandles())
      return(INIT_FAILED);

   return(INIT_SUCCEEDED);
  }

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
void OnDeinit(const int reason)
  {
   Print("Shutting down ", expertAdvisorName, "...");
   Print("Current Account Balance: ", AccountInfoDouble(ACCOUNT_BALANCE));
  }

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
void OnTick()
  {
   ticksReceivedCount++;
   string indicatorMetrics = "";

   //Trading Loop
   for(int symbolLoop = 0; symbolLoop < numberOfTradeableSymbols; symbolLoop++)
     {
      string currentIndicatorValues;

      //Get Open Signal
      string OpenSignalStatus = CheckForOpenSignal(symbolLoop, currentIndicatorValues);
      StringConcatenate(indicatorMetrics, indicatorMetrics, symbolArray[symbolLoop], "  |  ", currentIndicatorValues, "  |  OPEN_STATUS = ", OpenSignalStatus, "  |  ");
      
      //Get Modify Position Signal
      string ModifyPositionSignalStatus = CheckForModifyPositionSignal(symbolLoop);
      StringConcatenate(indicatorMetrics, indicatorMetrics, "MODIFY_POSITION_STATUS = ", ModifyPositionSignalStatus, "\n\r");
            
      //Get Close Partial Signal
      string ClosePartialSignalStatus = CheckForClosePartialSignal(symbolLoop);
      StringConcatenate(indicatorMetrics, indicatorMetrics, "CLOSE_PARTIAL_STATUS = ", ClosePartialSignalStatus, "\n\r");

      //Get Close Signal
      string CloseSignalStatus = CheckForCloseSignal(symbolLoop);
      StringConcatenate(indicatorMetrics, indicatorMetrics, "CLOSE_STATUS = ", CloseSignalStatus, "\n\r");
      
      //Calculate Position Volume
      double OpenTradeVolume = CalculatePositionVolume(symbolLoop, OpenSignalStatus);

//+--------------------------------------------------------------------------------+

      //Process Open Trade
      if ((OpenSignalStatus == "LONG" || OpenSignalStatus == "SHORT") && PositionsTotal() == 0)
         OpenPosition(symbolLoop, OpenSignalStatus, OpenTradeVolume);

      //Process Modify Position
      if (ModifyPositionSignalStatus == "MODIFY_LONG_FIRST" || ModifyPositionSignalStatus == "MODIFY_LONG_SECOND" ||
          ModifyPositionSignalStatus == "MODIFY_SHORT_FIRST" || ModifyPositionSignalStatus == "MODIFY_SHORT_SECOND")
            ModifyPosition(symbolLoop, ModifyPositionSignalStatus);
                  
      //Process Close Partial Position
      if (ClosePartialSignalStatus == "CLOSE_PARTIAL_LONG_FIRST" || ClosePartialSignalStatus == "CLOSE_PARTIAL_LONG_SECOND" || 
          ClosePartialSignalStatus == "CLOSE_PARTIAL_SHORT_FIRST" || ClosePartialSignalStatus == "CLOSE_PARTIAL_SHORT_SECOND")
            ClosePositionPartial(symbolLoop, ClosePartialSignalStatus);

      //Process Close Position
      if ((CloseSignalStatus == "CLOSE_LONG" || CloseSignalStatus == "CLOSE_SHORT") && PositionsTotal() > 0)
         ClosePosition(symbolLoop, CloseSignalStatus);
         
      //Output Trades to Chart   
      if(!MQLInfoInteger(MQL_TESTER))
         OutputStatusToChart(indicatorMetrics);

     } // Loop through Signals and Position Actions

  } // void OnTick()

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
double OnTester()
  {
   double profitFactor = TesterStatistics(STAT_PROFIT_FACTOR);
   Print("Profit Factor: ", profitFactor);
   double expectedPayoff = TesterStatistics(STAT_EXPECTED_PAYOFF);
   Print("Expected Payoff: ", expectedPayoff);
   return(0);
  }

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
void OpenPosition(int symbolLoop, string openDirection, double positionVolume)
  {
   string currentSymbol = symbolArray[symbolLoop];
   ulong currentOrderTicket = openTradeOrderTicket[symbolLoop];
   
   //Symbol Information
   double ask = SymbolInfoDouble(currentSymbol, SYMBOL_ASK);
   double bid = SymbolInfoDouble(currentSymbol, SYMBOL_BID);

   //Calculate StopLoss/TakeProfit
   double ATR[];
   double passValuesATR = SymCopyBuffer(handleATR[symbolLoop], 0, ATR, numValuesNeeded, currentSymbol, "ATR");
   double currATR = ATR[1];
   double stopLossBuy = ask - currATR * stopLossFactor;
   double takeProfitBuy = ask + currATR * takeProfitFactor * normalTakeProfitModifier;
   double stopLossSell = bid + currATR * stopLossFactor;
   double takeProfitSell = bid - currATR * takeProfitFactor * normalTakeProfitModifier;
   
   //Open Trade Position 
   if (openDirection == "LONG")
      trade.PositionOpen(currentSymbol, ORDER_TYPE_BUY, positionVolume, ask, stopLossBuy, takeProfitBuy, "Sending Buy Order..");
   if (openDirection == "SHORT")
      trade.PositionOpen(currentSymbol, ORDER_TYPE_SELL, positionVolume, bid, stopLossSell, takeProfitSell, "Sending Sell Order..");
  }
  
//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
void ModifyPosition(int symbolLoop, string modifyDirection)
  {
   string currentSymbol = symbolArray[symbolLoop];
   ulong currentOrderTicket = openTradeOrderTicket[symbolLoop];
   
   if (PositionSelect(currentSymbol) == true)
      {
         double positionOpenPrice = NormalizeDouble(PositionGetDouble(POSITION_PRICE_OPEN), 3);
         double positionTakeProfit = NormalizeDouble(PositionGetDouble(POSITION_TP), 3);
         double positionBuyTakeProfit = NormalizeDouble(PositionGetDouble(POSITION_TP) - positionOpenPrice, 5);
         double positionSellTakeProfit = NormalizeDouble(positionOpenPrice - PositionGetDouble(POSITION_TP), 5);

         //Close Trade Position Partial
         if(modifyDirection == "MODIFY_LONG_FIRST")
            {
               trade.PositionModify(currentSymbol, positionOpenPrice, positionTakeProfit + positionBuyTakeProfit);
               Print(expertAdvisorName, ": Modified Buy Position (", currentSymbol, "). Set Stop Loss to Open Price.");
            }
         else if(modifyDirection == "MODIFY_LONG_SECOND")
            {
               trade.PositionModify(currentSymbol, positionOpenPrice + positionBuyTakeProfit / 2, positionTakeProfit + positionBuyTakeProfit / 2);
               Print(expertAdvisorName, ": Modified Buy Position (", currentSymbol, "). Set Stop Loss to First Take Profit.******************");
            }
         else if(modifyDirection == "MODIFY_SHORT_FIRST")
            {
               trade.PositionModify(currentSymbol, positionOpenPrice, positionTakeProfit - positionSellTakeProfit);
               Print(expertAdvisorName, ": Modified Sell Position (", currentSymbol, "). Set Stop Loss to Open Price.");
            }
         else if(modifyDirection == "MODIFY_SHORT_SECOND")
            {
               trade.PositionModify(currentSymbol, positionOpenPrice - positionSellTakeProfit / 2, positionTakeProfit - positionSellTakeProfit / 2);
               Print(expertAdvisorName, ": Modified Sell Position (", currentSymbol, "). Set Stop Loss to First Take Profit.********************");
            }
      }
  }
  
//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
void ClosePositionPartial(int symbolLoop, string closeDirection)
  {
   string currentSymbol = symbolArray[symbolLoop];
   ulong currentOrderTicket = openTradeOrderTicket[symbolLoop];
   
   if (PositionSelect(currentSymbol) == true)
      {
         double positionVolume = PositionGetDouble(POSITION_VOLUME);
         double symbolPoint = SymbolInfoDouble(currentSymbol, SYMBOL_POINT);
         
         //Close Trade Position Partial
         if(closeDirection == "CLOSE_PARTIAL_LONG_FIRST")
            {
               partialCloseFactor = 4;
               double positionClose = NormalizeDouble(positionVolume / partialCloseFactor, 2);
               if (positionClose == 0)
                  positionClose = 1000 * symbolPoint;
                  
               trade.PositionClosePartial(currentSymbol, positionClose, -1);
               Print(expertAdvisorName, ": First Partial Close for Buy Position (", currentSymbol, "). PartialCloseFactor == ", partialCloseFactor);
            }
         else if(closeDirection == "CLOSE_PARTIAL_LONG_SECOND")
            {
               partialCloseFactor = 2;
               double positionClose = NormalizeDouble(positionVolume / partialCloseFactor, 2);
               if (positionClose == 0)
                  positionClose = 1000 * symbolPoint;
                  
               trade.PositionClosePartial(currentSymbol, positionClose, -1);
               Print(expertAdvisorName, ": Second Partial Close for Buy Position (", currentSymbol, "). PartialCloseFactor == ", partialCloseFactor);
            }
         else if(closeDirection == "CLOSE_PARTIAL_SHORT_FIRST")
            {
               partialCloseFactor = 4;
               double positionClose = NormalizeDouble(positionVolume / partialCloseFactor, 2);
               if (positionClose == 0)
                  positionClose = 1000 * symbolPoint;  
                              
               trade.PositionClosePartial(currentSymbol, positionClose, -1);
               Print(expertAdvisorName, ": First Partial Close for Sell Position (", currentSymbol, "). PartialCloseFactor == ", partialCloseFactor);
            }
         else if(closeDirection == "CLOSE_PARTIAL_SHORT_SECOND")
            {
               partialCloseFactor = 2;
               double positionClose = NormalizeDouble(positionVolume / partialCloseFactor, 2);
               if (positionClose == 0)
                  positionClose = 1000 * symbolPoint;
                  
               trade.PositionClosePartial(currentSymbol, positionClose, -1);
               Print(expertAdvisorName, ": Second Partial Close for Sell Position (", currentSymbol, "). PartialCloseFactor == ", partialCloseFactor);
            }
      }
  }
   
//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
void ClosePosition(int symbolLoop, string closeDirection)
  {
   string currentSymbol = symbolArray[symbolLoop];
   ulong currentOrderTicket = openTradeOrderTicket[symbolLoop];
   
   if (PositionSelect(currentSymbol) == true)
      { 
         //Close Trade Position
         if(closeDirection == "CLOSE_LONG")
            {
               trade.PositionClose(currentSymbol, -1);
               Print(expertAdvisorName, ": Closed Buy Position (", currentSymbol, ").");
            }
         if(closeDirection == "CLOSE_SHORT")
            {
               trade.PositionClose(currentSymbol, -1);
               Print(expertAdvisorName, ": Closed Sell Position (", currentSymbol, ").");
            }
      }
  }

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
double CalculatePositionVolume(int symbolLoop, string openDirection)
  {
   string currentSymbol = symbolArray[symbolLoop];
   ulong currentOrderTicket = openTradeOrderTicket[symbolLoop];
      
   //Account Information
   double accountBalance = AccountInfoDouble(ACCOUNT_BALANCE);
   double accountEquity = AccountInfoDouble(ACCOUNT_EQUITY);
   
   //Symbol Information
   double symbolPoint = SymbolInfoDouble(currentSymbol, SYMBOL_POINT);
   double ask = SymbolInfoDouble(currentSymbol, SYMBOL_ASK);
   double bid = SymbolInfoDouble(currentSymbol, SYMBOL_BID);
   
   //Calculate StopLoss
   double ATR[];
   double passValuesATR = SymCopyBuffer(handleATR[symbolLoop], 0, ATR, numValuesNeeded, currentSymbol, "ATR");
   double currATR = ATR[1];
   double stopLossBuy = currATR * stopLossFactor;
   double stopLossSell = currATR * stopLossFactor;
   
   //Calculate Lot Size
   if (selectLotSize == "Standard Lot")
      lotSize = 100000;
   else if (selectLotSize == "Mini Lot")
      lotSize = 10000;
   else if (selectLotSize == "Micro Lot")
      lotSize = 1000;
   else if (selectLotSize == "Nano Lot")
      lotSize = 100;
   else if (selectLotSize == "Pico Lot")
      lotSize = 10;
   else
      lotSize = 0;
      
   //Calculate Value per Pip
   double pipValueBuy = symbolPoint * lotSize / ask;
   double pipValueSell = symbolPoint * lotSize / bid;
   
   //Determine Position Size
   if (openDirection == "LONG")
      {
         positionSize = NormalizeDouble((accountEquity * riskFactor / stopLossBuy * pipValueBuy), 2);
         //Print(positionSize);
      }        
   else if (openDirection == "SHORT")
      {
         positionSize = NormalizeDouble((accountEquity * riskFactor / stopLossSell * pipValueSell), 2);
         //Print(positionSize);
      }
   
   //Prevent PositionSize=NULL Error
   if (positionSize == 0)
      if (symbolPoint == 4)
         positionSize = 1000 * symbolPoint;
      else if (symbolPoint == 3)
         positionSize = 100 * symbolPoint;
      else if (symbolPoint == 2)
         positionSize = 100 * symbolPoint;
      else
         positionSize = 1000 * symbolPoint;
         
   return positionSize;
  }

//+------------------------------------------------------------------+




//+--------------------------------------------------------------------------------+
//|                                                            SilentExpertAdvisor |
//|                                                    Copyright 2020, CompanyName |
//|                                                     http://www.companyname.net |
//+--------------------------------------------------------------------------------+
#property strict

#include <SilentInclude\Global Inputs.mqh>

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
void GetInitializeInformation()
   {   
      if (selectSymbol == "CURRENT")
         {
            numberOfTradeableSymbols = 1;
            symbolArray[0] = _Symbol;
            ArrayResize(symbolArray, 1);
            Print(expertAdvisorName, " will process ", symbolArray[0], " only.");
         }
      else
         {
            string tradeSymbolsToUse = "";
            if (selectSymbol == "ALL")
                  tradeSymbolsToUse = allSymbols;
            else tradeSymbolsToUse = selectSymbol;
            
            numberOfTradeableSymbols = StringSplit(tradeSymbolsToUse, '|', symbolArray);
            Print(expertAdvisorName, " will process ", tradeSymbolsToUse, "."); 
         }
   }

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+  
bool NewBar(const bool print_log=true)
  {
   static datetime barTime;
   datetime currentTime = iTime(selectSymbol, selectTimeFrame, selectShift);
   if(barTime != currentTime)
     {
      barTime = currentTime;
      if(print_log && !(MQLInfoInteger(MQL_OPTIMIZATION) || MQLInfoInteger(MQL_TESTER)))
        {
         MqlTick lastTick;
         if (!SymbolInfoTick(selectSymbol, lastTick))
            Print("SymbolInfoTick() failed", GetLastError());
        }
      return (true);
     }
   return (false);
  }

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+  
void ResizeCoreArrays()
   {
      ArrayResize(openTradeOrderTicket, numberOfTradeableSymbols);
   }
   
void ResizeIndicatorHandleArrays()
   {
      ArrayResize(handleWPR, numberOfTradeableSymbols);
      ArrayResize(handleWPRMA, numberOfTradeableSymbols);
      ArrayResize(handleMA, numberOfTradeableSymbols);
      ArrayResize(handleFORCE, numberOfTradeableSymbols);
      ArrayResize(handleATR, numberOfTradeableSymbols);
   }

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
bool SetUpIndicatorHandles()
   {
   for(int symbolLoop = 0; symbolLoop < numberOfTradeableSymbols; symbolLoop++)
      {
         //Reset previous Errors
         ResetLastError();
         
         handleATR[symbolLoop] = iATR(symbolArray[symbolLoop], selectTimeFrame, selectATRPeriod);
         handleFORCE[symbolLoop] = iForce(symbolArray[symbolLoop], selectTimeFrame, selectFORCEPeriod, selectFORCEMode, selectFORCEAppliedVolume);
         handleWPR[symbolLoop] = iWPR(symbolArray[symbolLoop], selectTimeFrame, selectWPRPeriod);         
         handleMA[symbolLoop] = iMA(symbolArray[symbolLoop], selectTimeFrame, selectMAPeriod, selectMAShift, selectMAMode, selectMAAppliedPrice);
         handleWPRMA[symbolLoop] = iMA(symbolArray[symbolLoop], selectTimeFrame, selectMAPeriod, selectMAShift, selectMAMode, handleWPR[symbolLoop]);
         
         if (handleATR[symbolLoop] == INVALID_HANDLE || handleFORCE[symbolLoop] == INVALID_HANDLE || handleWPR[symbolLoop] == INVALID_HANDLE || handleMA[symbolLoop] == INVALID_HANDLE || handleWPRMA[symbolLoop] == INVALID_HANDLE)
            {
               if (GetLastError() == 4302)
                  Print("Symbol needs to be added to the MarketWatch.");
               else Print("Error(INVALID_HANDLE): ", GetLastError());
                  return false;
            }
         Print("Handles for ", symbolArray[symbolLoop], " has been successfully created.");
      }
    return true;
   }

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
string CheckForOpenSignal(int symbolLoop, string& signalDiagnosticMetrics)
   {
    string currentSymbol = symbolArray[symbolLoop];
    ulong currentOrderTicket = openTradeOrderTicket[symbolLoop];
      
    //Copy Values from indicator buffers to local buffers
    double ATR[], FORCE[], MA[], WPR[], WPRMA[];
   
    double passValuesATR = SymCopyBuffer(handleATR[symbolLoop], 0, ATR, numValuesNeeded, currentSymbol, "ATR");
    double passValuesFORCE = SymCopyBuffer(handleFORCE[symbolLoop], 0, FORCE, numValuesNeeded, currentSymbol, "FORCE");
    double passValuesMA = SymCopyBuffer(handleMA[symbolLoop], 0, MA, numValuesNeeded, currentSymbol, "MA");
    double passValuesWPR = SymCopyBuffer(handleWPR[symbolLoop], 0, WPR, numValuesNeeded, currentSymbol, "WPR");
    double passValuesWPRMA = SymCopyBuffer(handleWPRMA[symbolLoop], 0, WPRMA, numValuesNeeded, currentSymbol, "WPRMA");
   
    double currWPR = WPR[1];
    double currFORCE = FORCE[1];
    double currMA = MA[1];
    double currATR = ATR[1];
    double currWPRMA = WPRMA[1];
    double lastWPR = WPR[2];
    double lastFORCE = FORCE[2];
    double lastMA = MA[2];
    double lastATR = ATR[2];
    double lastWPRMA = WPRMA[2];
   
    StringConcatenate(signalDiagnosticMetrics, "Average True Range = ", DoubleToString(currATR, (int)SymbolInfoInteger(currentSymbol, SYMBOL_DIGITS)),
                       "  |  Force Index = ", DoubleToString(currFORCE, (int)SymbolInfoInteger(currentSymbol, SYMBOL_DIGITS)),
                       "  |  Moving Average = " + DoubleToString(currMA, (int)SymbolInfoInteger(currentSymbol, SYMBOL_DIGITS)), 
                       "  |  William %R = " + DoubleToString(currWPR, (int)SymbolInfoInteger(currentSymbol, SYMBOL_DIGITS)), 
                       "  |  William %R Moving Average = " + DoubleToString(currWPRMA, (int)SymbolInfoInteger(currentSymbol, SYMBOL_DIGITS)));

    //Creating Long Signal
    if (currWPR > -20 && lastWPR < -20)
      signalWPRLong = 1;
    if (currWPR > currWPRMA)
      signalWPRMALong = 1; 
    if (currMA > lastMA)
      signalMALong = 1;
      
    //Creating Short Signal
    if (currWPR < -25 && lastWPR > -25)
      signalWPRShort = 1;
    if (currWPR < currWPRMA)
      signalWPRMAShort = 1;  
    if (currMA < lastMA)
      signalMAShort = 1;
    
    //Creating Open Signal
    if (signalWPRLong == 1 && signalWPRMALong == 1 && signalMALong == 1)
       return ("LONG");
    else if (signalWPRShort == 1 && signalWPRMAShort == 1 && signalMAShort == 1)
       return ("SHORT");
    return ("NO_SIGNAL");
   }
   
//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+  
string CheckForModifyPositionSignal(int symbolLoop)
   {
    string currentSymbol = symbolArray[symbolLoop];
    ulong currentOrderTicket = openTradeOrderTicket[symbolLoop];
   
    if (PositionSelect(currentSymbol) == true)
       { 
          //Position Information
          double positionOpenPrice = PositionGetDouble(POSITION_PRICE_OPEN);
          double positionCurrentPrice = PositionGetDouble(POSITION_PRICE_CURRENT);
          double positionStopLoss = PositionGetDouble(POSITION_SL);
          double positionBuyTakeProfit = PositionGetDouble(POSITION_TP) - positionOpenPrice;
          double positionSellTakeProfit = positionOpenPrice - PositionGetDouble(POSITION_TP);
          
          //Creating Close Partial Signal
          if (PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY)
            {
                bool secondModifyPermission = false;
                if (positionOpenPrice == positionStopLoss)
                  secondModifyPermission = true;
                
                if (((positionOpenPrice + positionBuyTakeProfit / 2 / secondTradeModifier) <= positionCurrentPrice) && secondModifyPermission == true)
                   return ("MODIFY_LONG_SECOND");
                else if ((positionOpenPrice + positionBuyTakeProfit / firstTradeModifier) <= positionCurrentPrice)
                   return ("MODIFY_LONG_FIRST");
            }
          else if (PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL)
            {
                bool secondModifyPermission = false;
                if (positionOpenPrice == positionStopLoss)
                  secondModifyPermission = true;            
                  
                if (((positionOpenPrice - positionSellTakeProfit / 2 / secondTradeModifier) >= positionCurrentPrice) && secondModifyPermission == true)
                   return ("MODIFY_SHORT_SECOND");
                else if ((positionOpenPrice - positionSellTakeProfit / firstTradeModifier) >= positionCurrentPrice)
                   return ("MODIFY_SHORT_FIRST");
            }
          return ("NO_SIGNAL");
       }
    return ("NO_SIGNAL");
   }
   
//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
string CheckForClosePartialSignal(int symbolLoop)
   {
    string currentSymbol = symbolArray[symbolLoop];
    ulong currentOrderTicket = openTradeOrderTicket[symbolLoop];
    
    if (PositionSelect(currentSymbol) == true)
      {
          //Position Information
          double positionOpenPrice = PositionGetDouble(POSITION_PRICE_OPEN);
          double positionCurrentPrice = PositionGetDouble(POSITION_PRICE_CURRENT);
          double positionStopLoss = PositionGetDouble(POSITION_SL);
          double positionBuyTakeProfit = PositionGetDouble(POSITION_TP) - positionOpenPrice;
          double positionSellTakeProfit = positionOpenPrice - PositionGetDouble(POSITION_TP);
        
          //Creating Close Partial Signal
          if (PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY)
            {
                bool secondModifyPermission = false;
                if (positionOpenPrice == positionStopLoss)
                  secondModifyPermission = true;
                  
                if (((positionOpenPrice + positionBuyTakeProfit / 2 / secondTradeModifier) <= positionCurrentPrice) && secondModifyPermission)
                   return ("CLOSE_PARTIAL_LONG_SECOND");  
                else if ((positionOpenPrice + positionBuyTakeProfit / firstTradeModifier) <= positionCurrentPrice)
                   return ("CLOSE_PARTIAL_LONG_FIRST");
            }
          else if (PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL)
            {
                bool secondModifyPermission = false;
                if (positionOpenPrice == positionStopLoss)
                  secondModifyPermission = true;
                           
                if (((positionOpenPrice - positionSellTakeProfit / 2 / secondTradeModifier) >= positionCurrentPrice) && secondModifyPermission)
                   return ("CLOSE_PARTIAL_SHORT_SECOND");
                else if ((positionOpenPrice - positionSellTakeProfit / firstTradeModifier) >= positionCurrentPrice)
                   return ("CLOSE_PARTIAL_SHORT_FIRST");
            }
          return ("NO_SIGNAL");
      }
    return ("NO_SIGNAL");
   }
   
//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
string CheckForCloseSignal(int symbolLoop)
   {
    string currentSymbol = symbolArray[symbolLoop];
    ulong currentOrderTicket = openTradeOrderTicket[symbolLoop];
    
    if (PositionSelect(currentSymbol) == true)
      {
          //Copy Values from indicator buffers to local buffers
          double ATR[], FORCE[], MA[], WPR[], WPRMA[];
         
          double passValuesATR = SymCopyBuffer(handleATR[symbolLoop], 0, ATR, numValuesNeeded, currentSymbol, "ATR");
          double passValuesFORCE = SymCopyBuffer(handleFORCE[symbolLoop], 0, FORCE, numValuesNeeded, currentSymbol, "FORCE");
          double passValuesMA = SymCopyBuffer(handleMA[symbolLoop], 0, MA, numValuesNeeded, currentSymbol, "MA");
          double passValuesWPR = SymCopyBuffer(handleWPR[symbolLoop], 0, WPR, numValuesNeeded, currentSymbol, "WPR");
          double passValuesWPRMA = SymCopyBuffer(handleWPRMA[symbolLoop], 0, WPRMA, numValuesNeeded, currentSymbol, "WPRMA");
         
          double currWPR = WPR[1];
          double currFORCE = FORCE[1];
          double currMA = MA[1];
          double currATR = ATR[1];
          double currWPRMA = WPRMA[1];
          double lastWPR = WPR[2];
          double lastFORCE = FORCE[2];
          double lastMA = MA[2];
          double lastATR = ATR[2];
          double lastWPRMA = WPRMA[2];
          
          //Creating Close Partial Signal
          if (PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY)
            {
                if (currWPR < -20 && lastWPR > -20)
                   return ("CLOSE_LONG");
            }
          else if (PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL)
            {  
                if (currWPR > -80 && lastWPR < -80)
                   return ("CLOSE_SHORT");
                else if (currWPR > -20 && lastWPR < -20)
                   return ("CLOSE_SHORT");
            }
          return ("NO_SIGNAL");
      }
    return ("NO_SIGNAL");
   }
 
//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+   
bool SymCopyBuffer(int ind_handle,
                   int buffer_num,
                   double &localArray[],
                   int numBarsRequired,
                   string symbolDescription,
                   string indDesc)
   {
      int availableBars;
      bool statusComplete = false;
      int failureCount = 0;
      
      while(!statusComplete)
         {
            availableBars = BarsCalculated(ind_handle);
            if (availableBars < numBarsRequired)
               {
                  failureCount++;
                  if (failureCount >= 3)
                     {
                        Print("Failed to calculate sufficient bars in tlamCopyBuffer() after ", failureCount, " attempts (", symbolDescription, "/", indDesc, " - Required = ", numBarsRequired, " Available = ", availableBars, ")");
                        return false;
                     }
                  Print("Attempt ", failureCount, ": Insufficient bars calculated for ", symbolDescription, "/", indDesc, "(Required = ", numBarsRequired, " Available = ", availableBars, ")");
                  Sleep(100);
               }
            else
               {
                  statusComplete = true;
                  if (failureCount > 0)
                     Print("Succeeded on attempt ", failureCount + 1);
               }
         }
         
         ResetLastError();
         
         int numAvailableBars = CopyBuffer(ind_handle, buffer_num, 0, numBarsRequired, localArray);
         if (numAvailableBars != numBarsRequired)
            {
               Print("Failed to copy data from indicator with error code ", GetLastError());
               return false;
            }
            
         ArraySetAsSeries(localArray, true);
         return true;    
   }

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+ 
void OutputStatusToChart(string additionalMetrics)
   {      
    //GMT Offset in Hours of MT5 Server
    double offsetInHours = (TimeCurrent() - TimeGMT()) / 3600.0;

    //Symbols being traded
    string symbolsText = "SYMBOLS BEING TRADED: ";
    for(int symbolLoop=0; symbolLoop < numberOfTradeableSymbols; symbolLoop++)
       StringConcatenate(symbolsText, symbolsText, " ", symbolArray[symbolLoop]);
   
    Comment("\n\rMT5 SERVER TIME: ", TimeCurrent(), " (OPERATING AT UTC/GMT", StringFormat("%+.1f", offsetInHours), ")\n\r\n\r",
             Symbol(), " TICKS RECEIVED: ", ticksReceivedCount, "\n\r\n\r",
             symbolsText,
             "\n\r\n\r", additionalMetrics);
   }