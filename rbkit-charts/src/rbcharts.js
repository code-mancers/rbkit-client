// top level rbkit which encapsulates all rbkit code. This also acts as
// namespace
var Rbkit = {
  // heap data which will be displayed as line chart
  heapData: {
    labels: ['-', '-'],
    datasets: [
        {
            label: 'Heap Objects',
            fillColor: "rgba(220,220,220,0.2)",
            strokeColor: "rgba(220,220,220,1)",
            pointColor: "rgba(220,220,220,1)",
            pointStrokeColor: "#fff",
            pointHighlightFill: "#fff",
            pointHighlightStroke: "rgba(220,220,220,1)",
            data: [0, 0]
        },
        {
            label: 'Heap Size',
            fillColor: "rgba(151,187,205,0.2)",
            strokeColor: "rgba(151,187,205,1)",
            pointColor: "rgba(151,187,205,1)",
            pointStrokeColor: "#fff",
            pointHighlightFill: "#fff",
            pointHighlightStroke: "rgba(151,187,205,1)",
            data: [0, 0]
        }
    ]
  },

  // gc data, which will be used in populating in gc graph
  gcData: {
    datasets: [{label: 'GC Data', data: [0]}],
    labels: [0]
  },

  // chart canvas contexts, maybe we can remove these
  heapDataCtx         : undefined,
  gcCtx               : undefined,
  youngGenerationCtx  : undefined,
  secondGenerationCtx : undefined,
  oldGenerationCtx    : undefined,

  // actual charts
  heapDataChart         : undefined,
  gcChart               : undefined,
  youngGenerationChart  : undefined,
  secondGenerationChart : undefined,
  oldGenerationChart    : undefined,

  // gc timings
  gcStartTime : undefined,
  gcCounter   : 0,

  // function to update gc graph
  updateGcChart: function (gcStarted, timestamp) {
  },

  // function to record gc start time
  gcStarted: function (timestamp) {
    this.gcStartTime = timestamp;
  },

  // function to record gc end time
  gcEnded: function (timestamp) {
    if (!this.gcStartTime) {
      console.info("not sure why gc start time is not set");
      return;
    }

    var gcDurationInMs = parseFloat(timestamp - this.gcStartTime);
    this.gcStartTime = undefined;

    this.gcChart.addData([gcDurationInMs], ++this.gcCounter);
    if (this.gcChart.datasets[0].bars.length > 10) {
      this.gcChart.removeData();
    }
    this.gcChart.update();
  },

  // set of polar colors which can be used
  polarChartDefaultColors: [
    // https://kuler.adobe.com/create/color-wheel/?base=2&rule=Analogous&selected=0&name=My%20Kuler%20Theme&mode=rgb&rgbvalues=1,0.8061476456696997,0.21226577883757825,0.91,0.3792784756568747,0.006674395779380451,1,0.05733450085646208,0.07395439645749258,0.6885308876037232,0.006674395779380451,0.91,0.007334500856462034,0.032410134950262015,1&swatchOrder=0,1,2,3,4
    "#FFCE36",
    "#E86102",
    "#FF0F13",
    "#B002E8",
    "#0208FF",

    // https://kuler.adobe.com/create/color-wheel/?base=2&rule=Analogous&selected=0&name=My%20Kuler%20Theme&mode=rgb&rgbvalues=0.04807254964605545,0.3841648710813974,1,0.026435686294499838,0.91,0.9023077127342737,0.07905020471923063,1,0.34320423970430325,0.48126239660538095,0.91,0.026435686294499838,1,0.8956645395163697,0.057458175950780066&swatchOrder=0,1,2,3,4
    "#0C62FF",
    "#07E8E6",
    "#14FF58",
    "#7BE807",
    "#FFE40F"
  ],

  // helper function to update a particular polar chart
  updatePolarChart: function(chart, newData) {
    // clear the whole chart first
    chart.segments = [];

    var iter = 0;
    for (var key in newData) {
      if (newData.hasOwnProperty(key)) {
        var color = this.polarChartDefaultColors[iter];
        var data = { value: newData[key], label: key, color: color, highlight: color };
        chart.addData(data);
        ++iter;
      }
    }

    chart.updateScaleRange(chart.segments);
    chart.scale.update();
    chart.update();
  },

  // function to update polar chart
  updateYoungGenerationChart: function (newData) {
    this.updatePolarChart(this.youngGenerationChart, newData);
  },

  updateSecondGenerationChart: function (newData) {
    this.updatePolarChart(this.secondGenerationChart, newData);
  },

  updateOldGenerationChart: function (newData) {
    this.updatePolarChart(this.oldGenerationChart, newData);
  },

  updateGcStats: function (gcStats) {
    for (var key in gcStats) {
      if (gcStats.hasOwnProperty(key)) {
        entry = document.getElementById(key);
        if (entry) {
          entry.textContent = gcStats[key];
        } else {
          var trNode = document.createElement('tr');
          var tdKey = document.createElement('td');
          tdKey.textContent = key;
          var tdVal = document.createElement('td');
          tdVal.id = key;
          tdVal.textContent = gcStats[key];
          trNode.appendChild(tdKey);
          trNode.appendChild(tdVal);

          // append created node to table
          document.getElementById('gc-stats-table')
            .firstElementChild
            .appendChild(trNode);
        }
      }
    }
  },

  updateHeapChart: function (newData) {
    var date = new Date();
    var timeStamp = date.getHours() + ':' + date.getMinutes() + ':' + date.getSeconds();

    var values = [newData['Heap Objects'], newData['Heap Size']];
    this.heapDataChart.addData(values, timeStamp);

    if (this.heapDataChart.datasets[0].points.length > 10) {
      this.heapDataChart.removeData();
    }

    this.heapDataChart.render();
  },

  init: function () {
    // charts for heap data
    var heapChartOptions = { showTooltips: false, animation: false };
    this.heapDataCtx   = document.getElementById('heap-chart').getContext('2d');
    this.heapDataChart = new Chart(this.heapDataCtx)
      .Line(this.heapData, heapChartOptions);

    // charts for gc stats.
    var gcChartOptions = { showTooltips: false, animation: false };
    this.gcCtx    = document.getElementById('gc-chart').getContext('2d');
    this.gcChart  = new Chart(this.gcCtx)
      .Bar(this.gcData, gcChartOptions);

    // charts for generations
    this.youngGenerationCtx  = document
      .getElementById('generation-one').getContext('2d');
    this.secondGenerationCtx = document
      .getElementById('generation-two').getContext('2d');
    this.oldGenerationCtx    = document
      .getElementById('generation-three').getContext('2d');

    var polarChartOptions = { showTooltips: false, animation: false };
    this.youngGenerationChart  = new Chart(this.youngGenerationCtx)
      .PolarArea([], polarChartOptions);
    this.secondGenerationChart = new Chart(this.secondGenerationCtx)
      .PolarArea([], polarChartOptions);
    this.oldGenerationChart    = new Chart(this.oldGenerationCtx)
      .PolarArea([], polarChartOptions);
  },

  receiveLiveData: function(data) {
    switch (data.event_type) {
    case "young_gen":
      this.updateYoungGenerationChart(data.payload);
      break;
    case "second_gen":
      this.updateSecondGenerationChart(data.payload);
      break;
    case "old_gen":
      this.updateOldGenerationChart(data.payload);
      break;
    case "gc_start":
      this.gcStarted(data.timestamp);
      break;
    case "gc_stop":
      this.gcEnded(data.timestamp);
      break;
    case "gc_stats":
      this.updateGcStats(data.payload);
      break;
    case "event_collection":
      this.updateHeapChart(data.payload);
      break;
    }
  }
};

Rbkit.init();
