var async     = require('async');
var assert    = require('assert');
var Remote    = require('divvy-lib').Remote;
var testutils = require('./testutils');
var config    = testutils.init_config();

suite('Account set', function() {
  var $ = { };

  setup(function(done) {
    testutils.build_setup().call($, function() {
      $.remote.local_signing = true;
      done();
    });
  });

  teardown(function(done) {
    testutils.build_teardown().call($, done);
  });

  test('null AccountSet', function(done) {
    var self = this;

    var steps = [
      function (callback) {
        self.what = 'Send null AccountSet';

        var transaction = $.remote.transaction().accountSet('root');
        transaction.setFlags(0);

        transaction.once('submitted', function(m) {
          assert.strictEqual(m.engine_result, 'tesSUCCESS');
          callback();
        });

        transaction.submit();
      },

      function (callback) {
        self.what = 'Check account flags';

        $.remote.requestAccountFlags('root', 'CURRENT', function(err, m) {
          assert.ifError(err);
          assert.strictEqual(m, 0);
          done();
        });
      }
    ]

    async.series(steps, function(err) {
      assert(!err, self.what + ': ' + err);
      done();
    });
  });

  test('set RequireDestTag', function(done) {
    var self = this;

    var steps = [
      function (callback) {
        self.what = 'Set RequireDestTag.';

        $.remote.transaction()
        .account_set('root')
        .set_flags('RequireDestTag')
        .on('submitted', function (m) {
          //console.log('proposed: %s', JSON.stringify(m));

          if (m.engine_result === 'tesSUCCESS') {
            callback(null);
          } else {
            //console.log(m);
            callback(new Error(m.engine_result));
          }
        })
        .submit();
      },

      function (callback) {
        self.what = 'Check RequireDestTag';

        $.remote.request_account_flags('root', 'CURRENT')
        .on('success', function (m) {
          var wrong = !(m.node.Flags & Remote.flags.account_root.RequireDestTag);

          if (wrong) {
            //console.log('Set RequireDestTag: failed: %s', JSON.stringify(m));
          }

          callback(wrong ? new Error(wrong) : null);
        })
        .request();
      },

      function (callback) {
        self.what = 'Clear RequireDestTag.';

        $.remote.transaction()
        .account_set('root')
        .set_flags('OptionalDestTag')
        .on('submitted', function (m) {
          //console.log('proposed: %s', JSON.stringify(m));
          callback(m.engine_result === 'tesSUCCESS' ? null : m.engine_result);
        })
        .submit();
      },

      function (callback) {
        self.what = 'Check No RequireDestTag';

        $.remote.request_account_flags('root', 'CURRENT')
        .on('success', function (m) {
          var wrong = !!(m.node.Flags & Remote.flags.account_root.RequireDestTag);

          if (wrong) {
            console.log('Clear RequireDestTag: failed: %s', JSON.stringify(m));
          }

          callback(wrong ? new Error(m) : null);
        })
        .request();
      }
    ]

    async.waterfall(steps,function (error) {
      assert(!error, self.what + ': ' + error);
      done();
    });
  });

  test('set RequireAuth',  function (done) {
    var self = this;

    var steps = [
      function (callback) {
        self.what = 'Set RequireAuth.';

        $.remote.transaction()
        .account_set('root')
        .set_flags('RequireAuth')
        .on('submitted', function (m) {
          //console.log('proposed: %s', JSON.stringify(m));
          callback(m.engine_result === 'tesSUCCESS' ? null : new Error(m));
        })
        .submit();
      },

      function (callback) {
        self.what = 'Check RequireAuth';

        $.remote.request_account_flags('root', 'CURRENT')
        .on('error', callback)
        .on('success', function (m) {
          var wrong = !(m.node.Flags & Remote.flags.account_root.RequireAuth);

          if (wrong) {
            console.log('Set RequireAuth: failed: %s', JSON.stringify(m));
          }

          callback(wrong ? new Error(m) : null);
        })
        .request();
      },

      function (callback) {
        self.what = 'Clear RequireAuth.';

        $.remote.transaction()
        .account_set('root')
        .set_flags('OptionalAuth')
        .on('submitted', function (m) {
          //console.log('proposed: %s', JSON.stringify(m));

          callback(m.engine_result !== 'tesSUCCESS');
        })
        .submit();
      },

      function (callback) {
        self.what = 'Check No RequireAuth';

        $.remote.request_account_flags('root', 'CURRENT')
        .on('error', callback)
        .on('success', function (m) {
          var wrong = !!(m.node.Flags & Remote.flags.account_root.RequireAuth);

          if (wrong) {
            console.log('Clear RequireAuth: failed: %s', JSON.stringify(m));
          }

          callback(wrong ? new Error(m) : null);
        })
        .request();
      }
      // XXX Also check fails if something is owned.
    ]

    async.waterfall(steps, function(error) {
      assert(!error, self.what + ': ' + error);
      done();
    });
  });

  test('set DisallowXDV', function(done) {
    var self = this;

    var steps = [
      function (callback) {
        self.what = 'Set DisallowXDV.';

        $.remote.transaction()
        .account_set('root')
        .set_flags('DisallowXDV')
        .on('submitted', function (m) {
          //console.log('proposed: %s', JSON.stringify(m));
          callback(m.engine_result === 'tesSUCCESS' ? null : new Error(m));
        })
        .submit();
      },

      function (callback) {
        self.what = 'Check DisallowXDV';

        $.remote.request_account_flags('root', 'CURRENT')
        .on('error', callback)
        .on('success', function (m) {
          var wrong = !(m.node.Flags & Remote.flags.account_root.DisallowXDV);

          if (wrong) {
            console.log('Set RequireDestTag: failed: %s', JSON.stringify(m));
          }

          callback(wrong ? new Error(m) : null);
        })
        .request();
      },

      function (callback) {
        self.what = 'Clear DisallowXDV.';

        $.remote.transaction()
        .account_set('root')
        .set_flags('AllowXDV')
        .on('submitted', function (m) {
          //console.log('proposed: %s', JSON.stringify(m));

          callback(m.engine_result === 'tesSUCCESS' ? null : new Error(m));
        })
        .submit();
      },

      function (callback) {
        self.what = 'Check AllowXDV';

        $.remote.request_account_flags('root', 'CURRENT')
        .on('error', callback)
        .on('success', function (m) {
          var wrong = !!(m.node.Flags & Remote.flags.account_root.DisallowXDV);

          if (wrong) {
            console.log('Clear DisallowXDV: failed: %s', JSON.stringify(m));
          }

          callback(wrong ? new Error(m) : null);
        })
        .request();
      }
    ]

    async.waterfall(steps, function(err) {
      assert(!err);
      done();
    });
  });
});
