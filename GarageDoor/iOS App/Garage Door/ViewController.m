//
//  ViewController.m
//  Garage Door
//
//  Created by Matthew Pierson on 21/10/20.
//

#import "ViewController.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

@interface ViewController ()

@end

@implementation ViewController

UIButton *button;
NSString *remoteIPAddress;
int remotePort;

- (void)viewDidLoad {
    [super viewDidLoad];
    remotePort = 20001;
    remoteIPAddress = @"192.168.1.112";
    // Do any additional setup after loading the view.
    button = [UIButton buttonWithType:UIButtonTypeSystem];
    [button setTitle:@"Open/Close Garage Door" forState:UIControlStateNormal];
    [button sizeToFit];
    button.backgroundColor = UIColor.systemGray3Color;
    [button setFrame:CGRectMake(10, 10, button.frame.size.width+20, button.frame.size.height+20)];
    [button setAutoresizingMask:UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin];
    [button.titleLabel setFont:[UIFont systemFontOfSize:14]];
    [button addTarget:self action:@selector(buttonPressed:) forControlEvents:UIControlEventTouchUpInside];
    button.center = self.view.center;
    button.layer.cornerRadius = 5;
    button.clipsToBounds = YES;
    [self.view addSubview:button];
}

-(void)buttonPressed:(UIButton *)button {
    [self broadCast];
}

- (void)broadCast
{    
    int socketSD = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketSD <= 0) {
        NSLog(@"Error: Could not open socket.");
        return;
    }
    // set socket options enable broadcast
    int broadcastEnable = 1;
    int ret = setsockopt(socketSD, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
    if (ret) {
        NSLog(@"Error: Could not open set socket to broadcast mode");
        close(socketSD);
        return;
    }
    // Configure the port and ip we want to send to
    struct sockaddr_in broadcastAddr;
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    inet_pton(AF_INET, [remoteIPAddress UTF8String], &broadcastAddr.sin_addr);
    broadcastAddr.sin_port = htons(20001);
    char *request = (char *)[@"MESSAGE_HERE" UTF8String];
    ret = (int)sendto(socketSD, request, strlen(request), 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
    if (ret < 0) {
        NSLog(@"Error: Could not open send broadcast.");
        close(socketSD);
        return;
    }
    close(socketSD);

}

- (IBAction)setIPButton:(UIButton *)sender {
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Please enter the IP Address:" message:@"e.g '192.168.1.111'" preferredStyle:UIAlertControllerStyleAlert];
        [alertController addTextFieldWithConfigurationHandler:^(UITextField * _Nonnull textField) {
            textField.placeholder = remoteIPAddress;
            textField.keyboardType=UIKeyboardTypeNumbersAndPunctuation;
            textField.secureTextEntry = NO;
        }];
        UIAlertAction *confirmAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        
            NSLog(@"Changing remoteIPAddress to: %@", [[alertController textFields][0] text]);
            remoteIPAddress = [[alertController textFields][0] text];

        }];
        [alertController addAction:confirmAction];
        UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
            NSLog(@"Cancelled");
            [self dismissViewControllerAnimated:YES completion:nil];
        }];
        [alertController addAction:cancelAction];
        [self presentViewController:alertController animated:YES completion:nil];
}

- (IBAction)setPortButton:(UIButton *)sender {
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:@"Please enter the Port:" message:@"e.g '20001'" preferredStyle:UIAlertControllerStyleAlert];
        [alertController addTextFieldWithConfigurationHandler:^(UITextField * _Nonnull textField) {
            textField.placeholder = [NSString stringWithFormat:@"\'%i\'", remotePort];
            textField.keyboardType=UIKeyboardTypeNumbersAndPunctuation;
            textField.secureTextEntry = NO;
        }];
        UIAlertAction *confirmAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        
            NSLog(@"Changing remotePort to: %@", [NSString stringWithFormat:@"\'%i\'", remotePort]);
            remotePort = (int)[alertController textFields][0];

        }];
        [alertController addAction:confirmAction];
        UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
            NSLog(@"Cancelled");
            [self dismissViewControllerAnimated:YES completion:nil];
        }];
        [alertController addAction:cancelAction];
        [self presentViewController:alertController animated:YES completion:nil];
}

@end
