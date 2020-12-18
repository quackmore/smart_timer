// home.js
$(document).ready(function () {
  esp_get_command_list().then(function () {
    hide_spinner(500);
  });
});

$('#commands_refresh').on('click', function () {
  show_spinner().then(function () {
    esp_get_command_list().then(function () {
      hide_spinner(500);
    })
  })
});

// Commands

function esp_get_command_list() {
  return esp_query({
    type: 'GET',
    url: '/api/command',
    dataType: 'json',
    success: update_commands_list,
    error: query_err
  });
}

function esp_get_command_idx(ii) {
  return esp_query({
    type: 'GET',
    url: '/api/command/' + ii,
    dataType: 'json',
    success: update_command_idx,
    error: query_err
  });
}

function esp_put_command_idx(ii) {
  return esp_query({
    type: 'PUT',
    url: '/api/command/' + ii,
    dataType: 'json',
    contentType: 'application/json',
    data: jsonify_command(),
    timeout: 8000,
    success: null,
    error: query_err
  });
}

function esp_post_command() {
  return esp_query({
    type: 'POST',
    url: '/api/command',
    dataType: 'json',
    contentType: 'application/json',
    data: jsonify_command(),
    timeout: 8000,
    success: null,
    error: query_err
  });
}

function esp_del_command_idx(ii) {
  return esp_query({
    type: 'DELETE',
    url: '/api/command/' + ii,
    dataType: 'json',
    contentType: 'application/json',
    timeout: 8000,
    success: null,
    error: query_err
  });
}

function command_enabled(val) {
  switch (val) {
    case 0:
      return '<i class="fa fa-square-o"></i>';
    case 1:
      return '<i class="fa fa-check-square-o"></i>';
    default:
      return '';
  }
}

function command_type(val) {
  switch (val) {
    case 1:
      return 'OPEN';
    case 2:
      return 'CLOSE';
    case 3:
      return 'pulse OPEN';
    case 4:
      return 'pulse CLOSE';
    default:
      return "";
  }
}

function command_duration(type, val) {
  if ((type == '3') || (type == '4'))
    return (val).toLocaleString('it');
  else
    return '-';
}

function when_dow(dow, dom, month) {
  switch (dow) {
    case 0:
      if ((dom == 0) && (month == 0))
        return 'day';
      else
        return '';
    case 1:
      if ((dom == 0) && (month == 0))
        return 'Mon';
      else
        return 'on Mon';
    case 2:
      if ((dom == 0) && (month == 0))
        return 'Tue';
      else
        return 'on Tue';
    case 3:
      if ((dom == 0) && (month == 0))
        return 'Wed';
      else
        return 'on Wed';
    case 4:
      if ((dom == 0) && (month == 0))
        return 'Thu';
      else
        return 'on Thu';
    case 5:
      if ((dom == 0) && (month == 0))
        return 'Fri';
      else
        return 'on Fri';
    case 6:
      if ((dom == 0) && (month == 0))
        return 'Sat';
      else
        return 'on Sat';
    case 7:
      if ((dom == 0) && (month == 0))
        return 'Sun';
      else
        return 'on Sun';
    default:
      return '';
  };
}

function when_month(dom, month) {
  if (dom == 0) {
    switch (month) {
      case 0:
        return 'every ';
      case 1:
        return 'every day on Jan ';
      case 2:
        return 'every day on Feb ';
      case 3:
        return 'every day on Mar ';
      case 4:
        return 'every day on Apr ';
      case 5:
        return 'every day on May ';
      case 6:
        return 'every day on Jun ';
      case 7:
        return 'every day on Jul ';
      case 8:
        return 'every day on Aug ';
      case 9:
        return 'every day on Sep ';
      case 10:
        return 'everu day on Oct ';
      case 11:
        return 'every day on Nov ';
      case 12:
        return 'every day on Dec ';
      default:
        return '';
    }
  }
  else {
    switch (month) {
      case 0:
        return 'every ' + dom + ' of month ';
      case 1:
        return 'every ' + dom + ' of Jan ';
      case 2:
        return 'every ' + dom + ' of Feb ';
      case 3:
        return 'every ' + dom + ' of Mar ';
      case 4:
        return 'every ' + dom + ' of Apr ';
      case 5:
        return 'every ' + dom + ' of May ';
      case 6:
        return 'every ' + dom + ' of Jun ';
      case 7:
        return 'every ' + dom + ' of Jul ';
      case 8:
        return 'every ' + dom + ' of Aug ';
      case 9:
        return 'every ' + dom + ' of Sep ';
      case 10:
        return 'everu ' + dom + ' of Oct ';
      case 11:
        return 'every ' + dom + ' of Nov ';
      case 12:
        return 'every ' + dom + ' of Dec ';
      default:
        return '';
    }
  }
}

function command_when(min, hour, dom, month, dow) {
  var when = '';
  if (min == -1) {
    if (hour == -1)
      return 'every minute ' + when_month(dom, month) + when_dow(dow, dom, month);
    else
      return 'at ' + ('0' + hour).slice(-2) + ':-- every minute ' + when_month(dom, month) + when_dow(dow, dom, month);
  }
  else {
    if (hour == -1)
      return 'at --:' + ('0' + min).slice(-2) + ' every hour ' + when_month(dom, month) + when_dow(dow, dom, month);
    else
      return 'at ' + ('0' + hour).slice(-2) + ':' + ('0' + min).slice(-2) + ' ' + when_month(dom, month) + when_dow(dow, dom, month);
  }
}

function update_commands_list(data) {
  var table_data = [];
  for (var ii = 0; ii < data.commands.length; ii++) {
    var obj = new Object();
    obj.en = command_enabled(data.commands[ii].enabled);
    obj.desc = data.commands[ii].name;
    obj.type = command_type(data.commands[ii].type);
    obj.contact = 'D' + data.commands[ii].relay_id;
    obj.duration = command_duration(data.commands[ii].type, data.commands[ii].duration);
    obj.when = command_when(data.commands[ii].min, data.commands[ii].hour, data.commands[ii].dom, data.commands[ii].month, data.commands[ii].dow);
    obj.actions = '<button class="btn btn-sm" onclick="modify_command(' +
      data.commands[ii].id +
      ')"><i class="fa fa-pencil-square-o"></i></button><button class="btn btn-sm" onclick="delete_command(' +
      data.commands[ii].id +
      ')"><i class="fa fa-trash-o"></i></button>';
    table_data.push(obj);
  }
  $("#commands_table").bootstrapTable({ data: table_data });
  $("#commands_table").bootstrapTable('load', table_data);
}

var current_id;

function update_command_idx(data) {
  $('#command_enabled').val(data.enabled);
  $('#command_name').val(data.name);
  $('#command_type').val(data.type);
  $('#command_duration').val(data.duration);
  if (($('#command_type').val() == 1) || ($('#command_type').val() == 2))
    $('#command_duration_group').addClass('d-none');
  else
    $('#command_duration_group').removeClass('d-none');
  $('#command_relay_id').val(data.relay_id);
  if (data.min == -1) {
    $('#command_min').val('');
    $('#command_min').prop('disabled', true);
    $('#every_min').prop('checked', true);
  }
  else {
    $('#command_min').val(data.min);
    $('#command_min').prop('disabled', false);
    $('#every_min').prop('checked', false);
  }
  if (data.hour == -1) {
    $('#command_hour').val('');
    $('#command_hour').prop('disabled', true);
    $('#every_hour').prop('checked', true);
  }
  else {
    $('#command_hour').val(data.hour);
    $('#command_hour').prop('disabled', false);
    $('#every_hour').prop('checked', false);
  }
  if (data.dom == 0) {
    $('#command_dom').val('');
    $('#command_dom').prop('disabled', true);
    $('#every_dom').prop('checked', true);
  }
  else {
    $('#command_dom').val(data.dom);
    $('#command_dom').prop('disabled', false);
    $('#every_dom').prop('checked', false);
  }
  if (data.month == 0) {
    $('#command_month').val('');
    $('#command_month').prop('disabled', true);
    $('#every_month').prop('checked', true);
  }
  else {
    $('#command_month').val(data.month);
    $('#command_month').prop('disabled', false);
    $('#every_month').prop('checked', false);
  }
  if (data.dow == 0) {
    $('#command_dow').val('');
    $('#command_dow').prop('disabled', true);
    $('#every_dow').prop('checked', true);
  }
  else {
    $('#command_dow').val(data.dow);
    $('#command_dow').prop('disabled', false);
    $('#every_dow').prop('checked', false);
  }
}

function modify_command(idx) {
  current_id = idx;
  $('#commandModalTitle').text('Command ' + current_id);
  $('#commandModalReset').prop('disabled', false);
  esp_get_command_idx(current_id).then(function () {
    $('#commandModal').modal('show');
  });
};

function delete_command(idx) {
  if (confirm("Deleted commands cannot be recovered.\nConfirm delete..."))
    show_spinner().then(function () {
      esp_del_command_idx(idx).then(function () {
        esp_get_command_list().then(function () {
          hide_spinner(500);
        })
      })
    })
};

$('#add_command').on('click', function () {
  current_id = -1;
  $('#commandModalTitle').text('New command');
  $('#command_enabled').val(1);
  $('#command_name').val("");
  $('#command_type').val(1);
  $('#command_duration_group').addClass('d-none');
  $('#command_min').val('');
  $('#command_min').prop('disabled', true);
  $('#every_min').prop('checked', true);
  $('#command_hour').val('');
  $('#command_hour').prop('disabled', true);
  $('#every_hour').prop('checked', true);
  $('#command_dom').val('');
  $('#command_dom').prop('disabled', true);
  $('#every_dom').prop('checked', true);
  $('#command_month').val('');
  $('#command_month').prop('disabled', true);
  $('#every_month').prop('checked', true);
  $('#command_dow').val('');
  $('#command_dow').prop('disabled', true);
  $('#every_dow').prop('checked', true);
  $('#commandModal').modal('show');
  $('#commandModalReset').prop('disabled', true);
});

$('#commandModalReset').on('click', function () {
  esp_get_command_idx(current_id);
});

$('#commandModalSave').on('click', function () {
  if (current_id < 0)
    esp_post_command().then(function () {
      $('#commandModal').modal('hide');
      show_spinner().then(function () {
        esp_get_command_list().then(function () {
          hide_spinner(500);
        })
      });
    });
  else
    esp_put_command_idx(current_id).then(function () {
      $('#commandModal').modal('hide');
      show_spinner().then(function () {
        esp_get_command_list().then(function () {
          hide_spinner(500);
        })
      });
    });
});

$('#command_name').on('change', function () {
  $('#command_name').val($('#command_name').val().slice(0, 31));
});

function validate_duration() {
  if (($('#command_duration').val() < 10))
    $('#command_duration').val(10);
  if (($('#command_duration').val() > 4294967295))
    $('#command_duration').val(4294967295);
}

$('#command_type').on('change', function () {
  validate_duration();
  if (($('#command_type').val() == 1) || ($('#command_type').val() == 2))
    $('#command_duration_group').addClass('d-none');
  else
    $('#command_duration_group').removeClass('d-none');
});

$('#command_duration').on('change', function () {
  validate_duration();
});

function validate_min() {
  if (($('#command_min').val() < 0))
    $('#command_min').val(0);
  if ($('#command_min').val() == '')
    $('#command_min').val('0');
  if (($('#command_min').val() > 59))
    $('#command_min').val(59);
}

$('#command_min').on('change', function () {
  validate_min();
});

$('#every_min').on('change', function () {
  validate_min();
  if ($('#every_min').prop('checked'))
    $('#command_min').prop('disabled', true);
  else
    $('#command_min').prop('disabled', false);
});

function validate_hour() {
  if (($('#command_hour').val() < 0))
    $('#command_hour').val(0);
  if ($('#command_hour').val() == '')
    $('#command_hour').val('0');
  if (($('#command_hour').val() > 23))
    $('#command_hour').val(23);
}

$('#command_hour').on('change', function () {
  validate_hour();
});

$('#every_hour').on('change', function () {
  validate_hour();
  if ($('#every_hour').prop('checked'))
    $('#command_hour').prop('disabled', true);
  else
    $('#command_hour').prop('disabled', false);
});

function validate_dom() {
  if (($('#command_dom').val() < 1))
    $('#command_dom').val(1);
  if (($('#command_dom').val() > 31))
    $('#command_dom').val(31);
}

$('#command_dom').on('change', function () {
  validate_dom();
});

$('#every_dom').on('change', function () {
  validate_dom();
  if ($('#every_dom').prop('checked'))
    $('#command_dom').prop('disabled', true);
  else
    $('#command_dom').prop('disabled', false);
});

function validate_month() {
  if (($('#command_month').val() < 1))
    $('#command_month').val(1);
  if (($('#command_month').val() > 12))
    $('#command_month').val(12);
}

$('#command_month').on('change', function () {
  validate_month();
  switch ($('#command_month').val()) {
    case 1, 3, 5, 7, 8, 10, 12:
      break;
    case 2:
      if (($('#command_dom').val() > 28))
        $('#command_dom').val(28);
      break;
    case 4, 6, 9, 11:
      if (($('#command_dom').val() > 30))
        $('#command_dom').val(30);
      break;
    default:
      break;
  }
});

$('#every_month').on('change', function () {
  validate_month();
  if ($('#every_month').prop('checked'))
    $('#command_month').prop('disabled', true);
  else
    $('#command_month').prop('disabled', false);
});

function validate_dow() {
  if (($('#command_dow').val() < 1))
    $('#command_dow').val(1);
  if (($('#command_dow').val() > 7))
    $('#command_dow').val(7);
}

$('#command_dow').on('change', function () {
  validate_dow();
});

$('#every_dow').on('change', function () {
  validate_dow();
  if ($('#every_dow').prop('checked'))
    $('#command_dow').prop('disabled', true);
  else
    $('#command_dow').prop('disabled', false);
});

function command_min_val() {
  if ($('#every_min').prop('checked'))
    return -1;
  else
    return parseInt($('#command_min').val());
}

function command_hour_val() {
  if ($('#every_hour').prop('checked'))
    return -1;
  else
    return parseInt($('#command_hour').val());
}

function command_dom_val() {
  if ($('#every_dom').prop('checked'))
    return 0;
  else
    return parseInt($('#command_dom').val());
}

function command_month_val() {
  if ($('#every_month').prop('checked'))
    return 0;
  else
    return parseInt($('#command_month').val());
}

function command_dow_val() {
  if ($('#every_dow').prop('checked'))
    return 0;
  else
    return parseInt($('#command_dow').val());
}

function jsonify_command() {
  return JSON.stringify({
    enabled: parseInt($('#command_enabled').val()),
    name: $('#command_name').val(),
    type: parseInt($('#command_type').val()),
    duration: parseInt($('#command_duration').val()),
    relay_id: parseInt($('#command_relay_id').val()),
    min: command_min_val(),
    hour: command_hour_val(),
    dom: command_dom_val(),
    month: command_month_val(),
    dow: command_dow_val()
  });
}