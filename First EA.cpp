//+--------------------------------------------------------------------------------+
//|                                                            SilentExpertAdvisor |
//|                                                    Copyright 2020, CompanyName |
//|                                                     http://www.companyname.net |
//+--------------------------------------------------------------------------------+
#property strict

//GLOBAL SETTINGS
input group                                           "Global Settings";
input string               selectSymbol               = "[Tester]";                               //Select Symbol
input ENUM_TIMEFRAMES      selectTimeFrame            = PERIOD_D1;                              //Select Timeframe
input ENUM_APPLIED_PRICE   selectAppliedPrice         = PRICE_CLOSE;                            //Select Applied Price
input int                  selectShift                = 0;                                      //Select Shift 

//MONEY MANAGMENT
input group                                           "Money Managment";
input int                  selectLotSize              = 100000;                                 //Select Lot Size (Full Lot, Micro Lot, ...)
input bool                 pipValueCalculate          = false;                                  //Standard Lot Size (= true --> ATR Lot Size = false)
input bool                 pipValueATRCalculate       = true;                                   //ATR Lot Size (= true --> Standard Lot Size = false)

//RISK MANAGMENT
input group                                           "Risk Managment";
input double               riskFactor                 = 0.02;                                   //Select Risk Factor (0.02 --> 2%)
input double               stopLossFactor             = 1.5;                                    //Stop Loss Factor
input double               takeProfitFactor           = 1.0;                                    //Take Profit Factor

//INDICATOR SETTINGS
input group                                           "William%R Settings";
input bool                 useWPR                     = true;                                   //Use WPR                                     
input int                  selectWPRPeriod            = 25;                                     //WPR Period

input group                                           "Force Index Settings";
input bool                 useFORCE                   = true;                                   //Use FORCE
input int                  selectFORCEPeriod          = 13;                                     //FORCE Period
input ENUM_MA_METHOD       selectFORCEMode            = MODE_SMA;                               //FORCE Mode
input ENUM_APPLIED_VOLUME  selectFORCEAppliedVolume   = VOLUME_TICK;                            //FORCE Volume

input group                                           "Moving Average Settings";
input bool                 useMA                      = true;                                   //Use MA
input int                  selectMAPeriod             = 7;                                      //MA Period
input int                  selectMAShift              = 0;                                      //MA Shift
input ENUM_MA_METHOD       selectMAMode               = MODE_SMA;                               //MA Mode
input ENUM_APPLIED_PRICE   selectMAAppliedPrice       = PRICE_CLOSE;                            //MA Applied Price

input group                                           "WPR Moving Average Settings";
input bool                 useWPRMA                   = true;                                   //Use WPRMA
input int                  selectWPRMAPeriod          = 10;                                     //WPRMA Period
input int                  selectWPRMAShift           = 0;                                      //WPRMA Shift
input ENUM_MA_METHOD       selectWPRMAMode            = MODE_SMA;                               //WPRMA Mode

input group                                           "Average True Range Settings";
input int                  selectATRPeriod            = 14;                                     //ATR Period

//INDICATOR HANDLES
int handleWPR, handleFORCE, handleMA, handleATR, handleWPRMA;

//ARRAYS
double WPR[], FORCE[], MA[], ATR[], WPRMA[];
double currWPR, currFORCE, currMA, currATR, currWPRMA;
double lastWPR, lastFORCE, lastMA, lastATR, lastWPRMA;

//TRADE INFORMATION
double ask, bid, stopLossBuy, stopLossSell, takeProfitBuy, takeProfitSell;
double pipValue, pipValueATR, positionSize, positionSizeATR;

//POSITION INFORMATION
int totalBuyPositions, totalSellPositions, positionDirection, positionTicket;
double accountBalance, accountEquity;



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
//INDICATOR HANDLES
      handleWPR = iWPR(selectSymbol, selectTimeFrame, selectWPRPeriod);
      handleFORCE = iForce(selectSymbol, selectTimeFrame, selectFORCEPeriod, selectFORCEMode, selectFORCEAppliedVolume);
      handleMA = iMA(selectSymbol, selectTimeFrame, selectMAPeriod, selectMAShift, selectMAMode, selectMAAppliedPrice);
      handleATR = iATR(selectSymbol, selectTimeFrame, selectATRPeriod);
      handleWPRMA = iMA(selectSymbol, selectTimeFrame, selectWPRMAPeriod, selectWPRMAShift, selectWPRMAMode, handleWPR);
   
//ARRAY SETTINGS
      ArraySetAsSeries(WPR, true);
      ArraySetAsSeries(FORCE, true);
      ArraySetAsSeries(MA, true);
      ArraySetAsSeries(ATR, true);
      ArraySetAsSeries(WPRMA, true);
      
      Print("InititalizeInformation successfully loaded.");
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
void GetTickInformation()
   {
//PRICE DEFINITIONS
      ask = NormalizeDouble(SymbolInfoDouble(selectSymbol, SYMBOL_ASK),_Digits);
      bid = NormalizeDouble(SymbolInfoDouble(selectSymbol, SYMBOL_BID),_Digits);

//ACCOUNT INFORMATION
      accountBalance = AccountInfoDouble(ACCOUNT_BALANCE);
      accountEquity = AccountInfoDouble(ACCOUNT_EQUITY);
          
//BUFFERS  
      CopyBuffer(handleWPR, 0, 0, 3, WPR);     
      CopyBuffer(handleFORCE, 0, 0, 3, FORCE);
      CopyBuffer(handleMA, 0, 0, 3, MA);
      CopyBuffer(handleATR, 0, 0, 3, ATR);
      CopyBuffer(handleWPRMA, 0, 0, 3, WPRMA);

//ARRAYS
      currWPR = WPR[1];
      currFORCE = FORCE[1];
      currMA = MA[1];
      currATR = ATR[1];
      currWPRMA = WPRMA[1];
      lastWPR = WPR[2];
      lastFORCE = FORCE[2];
      lastMA = MA[2];
      lastATR = ATR[2];
      lastWPRMA = WPRMA[2];

//IMPORTANT VALUES
      pipValue = _Point * selectLotSize * 10 / ask;
      pipValueATR = riskFactor / currATR * stopLossFactor;
      if (selectSymbol == "EURJPY")
         pipValueATR = riskFactor / currATR * stopLossFactor * 100;
       
//POSITION SIZE CALCULATION     
      positionSize = NormalizeDouble(accountBalance * _Point / currATR * stopLossFactor * pipValue, 5);
      positionSizeATR = NormalizeDouble(accountBalance * _Point / currATR * stopLossFactor * pipValueATR * 0.0001, 3);
      
      stopLossBuy = ask - currATR * stopLossFactor;
      takeProfitBuy = ask + currATR * takeProfitFactor;
      stopLossSell = bid + currATR * stopLossFactor;
      takeProfitSell = bid - currATR * takeProfitFactor;

//ACTIVE POSITION TRACKING         
      totalBuyPositions = 0;
      totalSellPositions = 0;
   }

 


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
      GetInitializeInformation();
      return(INIT_SUCCEEDED);
  }

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
void OnDeinit(const int reason)
  {
      Print("Shutting down SEA...");
      Print("Current Account Balance: ", accountBalance);
  }

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
void OnTick()
  {   
      if (NewBar())
        {  
            GetTickInformation();
            bool DefaultPosition = true;
            if(DefaultPosition == true)
               {
                  if (CheckForBuySignal())
                     OpenDefaultBuyPosition();
                  if (CheckForSellSignal())
                     OpenDefaultSellPosition();
                  if (CheckForBuyPositionClose())
                     CloseBuyPosition();
                  if (CheckForSellPositionClose())
                     CloseSellPosition();
               }
            
            if(DefaultPosition == false)
               {
                  if (CheckForBuySignal())
                     OpenConfiguratedBuyPosition();
                  if (CheckForSellSignal())
                     OpenConfiguratedSellPosition();
                  if (CheckForBuyPositionClose())
                     CloseBuyPosition();
                  if (CheckForSellPositionClose())
                     CloseSellPosition();
               }

            
        } // NewBar Function

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
bool CheckForBuySignal()
   {
      if (currWPR > -20 && lastWPR < -20)
         if (currWPR > currWPRMA)
            if (currMA > lastMA)
               return true;
      return false;    
   }
   
bool CheckForSellSignal()
   {
      if (currWPR < -25 && lastWPR > -25)
            return true;
      return false;      
   }
   
bool CheckForBuyPositionClose()
   {
      if (currWPR < -20 && lastWPR > -20)
            return true;
      return false;    
   } 
     
bool CheckForSellPositionClose()
   {
      if (currWPR > -80 && lastWPR < -80)
         return true;
      else if (currWPR > -20 && lastWPR < -20)
         return true;
      return false;
   }

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
void OpenDefaultBuyPosition()
   {
      trade.PositionOpen(selectSymbol, ORDER_TYPE_BUY, 0.01, ask, stopLossBuy, takeProfitBuy, "Sending Buy Order...");
      trade.PositionOpen(selectSymbol, ORDER_TYPE_BUY, 0.01, ask, stopLossBuy, takeProfitBuy + currATR * takeProfitFactor, "Sending Buy Order...");
   }
            
void OpenDefaultSellPosition()
   {
      trade.PositionOpen(selectSymbol, ORDER_TYPE_SELL, 0.01, ask, stopLossSell, takeProfitSell, "Sending Sell Order...");
      trade.PositionOpen(selectSymbol, ORDER_TYPE_SELL, 0.01, ask, stopLossSell, takeProfitSell - currATR * takeProfitFactor, "Sending Sell Order...");
   }

void OpenConfiguratedBuyPosition()
   {
      trade.PositionOpen(selectSymbol, ORDER_TYPE_BUY, 1, ask, stopLossBuy, takeProfitBuy, "Sending Buy Order...");
      trade.PositionOpen(selectSymbol, ORDER_TYPE_BUY, 1, ask, stopLossBuy, takeProfitBuy + currATR, "Sending Buy Order...");      
   }
   
void OpenConfiguratedSellPosition()
   {
      trade.PositionOpen(selectSymbol, ORDER_TYPE_SELL, 1, ask, 0, 0, "Sending Sell Order...");
      trade.PositionOpen(selectSymbol, ORDER_TYPE_SELL, 1, ask, 0, 0, "Sending Sell Order...");      
   }

//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
void CloseBuyPosition()
   {  
      PositionSelect(selectSymbol);
      if (positionDirection == POSITION_TYPE_BUY)
         {
            trade.PositionClose(selectSymbol, 10);
            if (PositionsTotal() == 2)
               Print("Buy Position closed");
               
         }
   }
   
void CloseSellPosition()
   {
      if (positionDirection == POSITION_TYPE_SELL)
         trade.PositionClose(selectSymbol, -1);
   }
      
//+--------------------------------------------------------------------------------+
//|                                                                                |
//+--------------------------------------------------------------------------------+
