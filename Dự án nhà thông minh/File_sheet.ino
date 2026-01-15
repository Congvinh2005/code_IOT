function doGet(e) { 
  Logger.log(JSON.stringify(e));
  var result = 'Ok';
  if (e.parameter == 'undefined') {
    result = 'No Parameters';
  }
  else {
    var sheet_id = '1RdtvOs_wiFTLAZcgnkXhQ2lIzC3nn8rpaZV5Zc1D0jk'; 
    var sheet_name = "DANHSACH"; 

    var sheet_open = SpreadsheetApp.openById(sheet_id);
    var sheet_target = sheet_open.getSheetByName(sheet_name);

    var newRow = sheet_target.getLastRow();

    var rowDataLog = [];

    var Curr_Date = Utilities.formatDate(new Date(), "Asia/Jakarta", 'dd/MM/yyyy');
    rowDataLog[0] = Curr_Date;  

    var Curr_Time = Utilities.formatDate(new Date(), "Asia/Jakarta", 'HH:mm:ss');
    rowDataLog[1] = Curr_Time;  

    var sts_val = '';

    for (var param in e.parameter) {
      Logger.log('In for loop, param=' + param);
      var value = stripQuotes(e.parameter[param]);
      Logger.log(param + ':' + e.parameter[param]);
      switch (param) {
        case 'sts':
          sts_val = value;
          break;

        case 'uid':
          rowDataLog[2] = value;  
          result += ', UID Written';
          break;

        case 'name':
          rowDataLog[3] = value; 
          result += ', Name Written';
          break; 

        case 'inout':
          rowDataLog[4] = value; 
          result += ', INOUT Written';
          break;       

        default:
          result += ", unsupported parameter";
      }
    }

    
    if (sts_val == 'writeuid') {
     
      Logger.log(JSON.stringify(rowDataLog));
      
     
      if (Array.isArray(rowDataLog) && rowDataLog.length > 2) {
        var RangeDataLatest = sheet_target.getRange('F1');
        RangeDataLatest.setValue(rowDataLog[2]);
        
        return ContentService.createTextOutput('Success');
      } else {
        Logger.log('Error: rowDataLog is not valid');
        return ContentService.createTextOutput('Error: Invalid data');
      }
    }
    

    if (sts_val == 'writelog') {
      sheet_name = "DIEMDANH";  
      sheet_target = sheet_open.getSheetByName(sheet_name);
      
      Logger.log(JSON.stringify(rowDataLog));
     
      sheet_target.insertRows(2);
      var newRangeDataLog = sheet_target.getRange(2,1,1, rowDataLog.length);
      newRangeDataLog.setValues([rowDataLog]);
      //maxRowData(11);
      return ContentService.createTextOutput(result);
    }
    
    
    if (sts_val == 'read') {
      sheet_name = "DANHSACH";  
      sheet_target = sheet_open.getSheetByName(sheet_name);

   
      var all_Data = sheet_target.getRange('A2:C11').getDisplayValues();
      
  
      return ContentService.createTextOutput(all_Data);
    }
  }
}
function maxRowData(allRowsAfter) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet()
                              .getSheetByName('DATA')
  
  sheet.getRange(allRowsAfter+1, 1, sheet.getLastRow()-allRowsAfter, sheet.getLastColumn())
       .clearContent()

}
function stripQuotes( value ) {
  return value.replace(/^["']|['"]$/g, "");
}
//________________